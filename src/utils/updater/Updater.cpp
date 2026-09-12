// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "Updater.h"

#include <common/util.h>
#undef config
#include <openpgp/hash.h>

#include "constants.h"
#include "Utils.h"
#include "utils/AsyncTask.h"
#include "utils/Networking.h"
#include "utils/SemanticVersion.h"

Updater::Updater(QObject *parent) :
    QObject(parent)
{
    // No maintainer key is shipped yet, so verifySignature() finds no key to
    // match and every candidate update is rejected. To enable updates: add the
    // ASCII-armored public key under src/assets/gpg_keys/, register it in
    // src/assets.qrc, and load it here.

    qDebug() << "Platform tag: " << this->getPlatformTag();
}

void Updater::checkForUpdates() {
    // No update-manifest endpoint exists for this project yet, so fail safely
    // rather than querying the one belonging to the project it was forked from.
    emit updateCheckFailed("Update checking is currently unavailable");
}

void Updater::onUpdateCheckResponse(QNetworkReply *reply) {
    const QString err = reply->errorString();

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonObject updates;
    if (!data.isEmpty() && Utils::validateJSON(data)) {
        auto doc = QJsonDocument::fromJson(data);
        updates = doc.object();
    }
    else {
        qWarning() << err;
        emit updateCheckFailed(err);
        return;
    }

    this->wsUpdatesReceived(updates);
}

void Updater::wsUpdatesReceived(const QJsonObject &updates) {
    // Disabled: the pushed payload describes releases of the upstream project,
    // not of BestWallet, so it must never be surfaced as an update.
    Q_UNUSED(updates);
}

void Updater::onSignedHashesReceived(QNetworkReply *reply, const QString &platformTag, const QString &version) {
    if (reply->error() != QNetworkReply::NoError) {
        QString err{QString("Unable to fetch signed hashed: %1").arg(reply->errorString())};
        emit updateCheckFailed(err);
        qWarning() << err;
        return;
    }

    QByteArray armoredSignedHashes = reply->readAll();
    reply->deleteLater();

    const QString binaryFilename = QString("%1-%2-%3.zip").arg(constants::releaseArtifactName, version, platformTag);
    QByteArray signedHash{};
    QString signer;
    try {
         signedHash = this->verifyParseSignedHashes(armoredSignedHashes, binaryFilename, signer);
    }
    catch (const std::exception &e) {
        QString err{QString("Failed to fetch and verify signed hash: %1").arg(e.what())};
        emit updateCheckFailed(err);
        qWarning() << err;
        return;
    }

    QString hash = signedHash.toHex();
    qInfo() << "Update found: " << binaryFilename << hash << "signed by:" << signer;

    this->state = Updater::State::UPDATE_AVAILABLE;
    this->version = version;
    this->binaryFilename = binaryFilename;
    this->downloadUrl = QString("%1/files/releases/%2/%3").arg(this->getWebsiteUrl(), platformTag, binaryFilename);
    this->hash = hash;
    this->signer = signer;
    this->platformTag = platformTag;

    emit updateAvailable();
}

QString Updater::getPlatformTag() {
#ifdef Q_OS_MACOS
    if (QString(BESTWALLET_TARGET_TRIPLET) == "arm64-apple-darwin") {
        return "mac-arm64";
    } else {
        return "mac";
    }
#endif
#ifdef Q_OS_WIN
    #ifdef PLATFORM_INSTALLER
    return "win-installer";
#endif
    return "win";
#endif
#ifdef Q_OS_LINUX
    QString tag = "";

    QString arch = QSysInfo::buildCpuArchitecture();
    if (arch == "arm64") {
        tag += "linux-arm64";
    } else if (arch == "arm") {
        tag += "linux-arm";
    } else if (arch == "riscv64") {
        tag += "linux-riscv64";
    } else {
        tag += "linux";
    }

    if (!qEnvironmentVariableIsEmpty("APPIMAGE")) {
        tag += "-appimage";
    }

#if !defined(HAS_TOR_BIN)
    tag += "-a";
#endif

    return tag;
#endif
    return "";
}

QString Updater::getWebsiteUrl() {
    // The project has no hidden service, so everyone gets the clearnet host.
    // Requests still go through whichever proxy is configured, and integrity
    // does not rest on the transport: the hashes are PGP-signed.
    return constants::websiteUrl;
}

QByteArray Updater::verifyParseSignedHashes(
        const QByteArray &armoredSignedHashes,
        const QString &binaryFilename,
        QString &signers) const
{
    const QString signedMessage = verifySignature(armoredSignedHashes, signers);

    return parseShasumOutput(signedMessage, binaryFilename);
}

QByteArray Updater::getHash(const void *data, size_t size) const
{
    QByteArray hash(sizeof(crypto::hash), 0);
    tools::sha256sum(static_cast<const uint8_t *>(data), size, *reinterpret_cast<crypto::hash *>(hash.data()));
    return hash;
}

QByteArray Updater::parseShasumOutput(const QString &message, const QString &filename) const
{
    for (const auto &line : message.split("\n"))
    {
        const auto trimmed = line.trimmed();
        if (trimmed.endsWith(filename))
        {
            const int pos = trimmed.indexOf(' ');
            if (pos != -1)
            {
                return QByteArray::fromHex(trimmed.left(pos).toUtf8());
            }
        }
        else if (trimmed.startsWith(filename))
        {
            const int pos = trimmed.lastIndexOf(' ');
            if (pos != -1)
            {
                return QByteArray::fromHex(trimmed.right(trimmed.size() - pos).toUtf8());
            }
        }
    }

    throw std::runtime_error("hash not found");
}

QString Updater::verifySignature(const QByteArray &armoredSignedMessage, QString &signer) const
{
    const std::string messageString = armoredSignedMessage.toStdString();

    const openpgp::message_armored signedMessage(messageString);
    signer = verifySignature(signedMessage, openpgp::signature_rsa::from_armored(messageString));

    const epee::span<const uint8_t> message = signedMessage;
    return QString(QByteArray(reinterpret_cast<const char *>(&message[0]), message.size()));
}

QString Updater::verifySignature(const epee::span<const uint8_t> data, const openpgp::signature_rsa &signature) const
{
    for (const auto &maintainer : m_maintainers)
    {
        for (const auto &public_key : maintainer)
        {
            try {
                if (signature.verify(data, public_key))
                {
                    return QString::fromStdString(maintainer.user_id());
                }
            }
            catch (const std::exception &e) {
                qWarning() << e.what();
            }
        }
    }

    throw std::runtime_error("not signed by a maintainer");
}
