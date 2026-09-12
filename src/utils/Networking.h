// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_NETWORKING_H
#define BESTWALLET_NETWORKING_H

#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "utils/Utils.h"

class Networking : public QObject
{
Q_OBJECT

public:
    explicit Networking(QObject *parent = nullptr);

    // Extra request headers, passed per call rather than held on the object, so
    // a shared instance cannot leak one request's credentials into the next.
    using Headers = QMap<QByteArray, QByteArray>;

    QNetworkReply* get(QObject *parent, const QString &url);
    QNetworkReply* getJson(QObject *parent, const QString &url, const Headers &headers = {});
    QNetworkReply* postJson(QObject *parent, const QString &url, const QJsonObject &data, const Headers &headers = {});
    void setUserAgent(const QString &userAgent);

private:
    QString m_userAgent = "Mozilla/5.0 (Windows NT 10.0; rv:102.0) Gecko/20100101 Firefox/102.0";
    QNetworkAccessManager *m_networkAccessManager;
};

#endif //BESTWALLET_NETWORKING_H
