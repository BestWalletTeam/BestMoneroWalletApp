// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "api/changenow.h"
#include "changenow_key.h"

#include <cmath>

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

#include "utils/config.h"

namespace {
    const QString kApiBase = QStringLiteral("https://api.changenow.io/v2");

    // Compiled in and therefore public: every user of every copy shares its
    // quota, and ChangeNOW can rate-limit or revoke it. Config::swapApiKey is
    // the way out without shipping a build.
    const QString kBundledApiKey = QStringLiteral(CHANGENOW_API_KEY);

    // Floating rate: the payout is settled when the deposit lands rather than
    // locked at order time.
    const QString kFlow = QStringLiteral("standard");

    // The amount named is the deposit, not the payout.
    const QString kType = QStringLiteral("direct");

    // How close a reverse solve's payout must land to the requested one.
    // Successive live quotes for the same amount jitter by roughly 0.01-0.04%,
    // so a tighter bound sits inside the noise and the solve spends its whole
    // budget chasing it -- sequential round-trips that are slowest over Tor.
    // Still far below how much a floating rate moves before settlement.
    constexpr double kReverseTolerance = 5e-4;

    // Upper bound on estimates per reverse solve, the minimum probe included.
    constexpr int kReverseMaxQuotes = 5;

    // Below this spread between two quoted deposits, the difference in their
    // payouts is mostly jitter, and a secant through them is not trusted.
    constexpr double kMinBaseline = 0.01;

    // The precision amounts are sent at; see formatAmount.
    double roundAmount(double amount) {
        return std::round(amount * 1e8) / 1e8;
    }

    // Plain decimal rather than QJsonValue's double formatting, which renders
    // small figures as "1e-05" and the API rejects those.
    QString formatAmount(double amount) {
        return QString::number(amount, 'f', 8);
    }
}

ChangeNowApi::ChangeNowApi(QObject *parent)
    : QObject(parent)
    , m_network(new Networking(this))
{
}

QString ChangeNowApi::apiKey() {
    const QString configured = ChangeNowApi::userApiKey();
    return configured.isEmpty() ? kBundledApiKey : configured;
}

QString ChangeNowApi::userApiKey() {
    return conf()->get(Config::swapApiKey).toString().trimmed();
}

bool ChangeNowApi::usingBundledKey() {
    return ChangeNowApi::userApiKey().isEmpty();
}

bool ChangeNowApi::hasApiKey() {
    return !ChangeNowApi::apiKey().isEmpty();
}

Networking::Headers ChangeNowApi::authHeaders() const {
    return {{QByteArrayLiteral("x-changenow-api-key"), ChangeNowApi::apiKey().toUtf8()}};
}

double ChangeNowApi::readAmount(const QJsonObject &obj, const QString &key) {
    const QJsonValue value = obj.value(key);
    if (value.isString()) {
        return value.toString().toDouble();
    }
    return value.toDouble();
}

QUrl ChangeNowApi::pairUrl(const QString &path, const Asset &from, const Asset &to) const {
    QUrlQuery query;
    query.addQueryItem("fromCurrency", from.ticker);
    query.addQueryItem("fromNetwork", from.network);
    query.addQueryItem("toCurrency", to.ticker);
    query.addQueryItem("toNetwork", to.network);
    query.addQueryItem("flow", kFlow);

    QUrl url{kApiBase + path};
    url.setQuery(query);
    return url;
}

bool ChangeNowApi::handledAsKeyFailure(QNetworkReply *reply) {
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 401) {
        return false;
    }
    emit keyRequired();
    return true;
}

void ChangeNowApi::requestRange(const Asset &from, const Asset &to) {
    if (!ChangeNowApi::hasApiKey()) {
        QMetaObject::invokeMethod(this, &ChangeNowApi::keyRequired, Qt::QueuedConnection);
        return;
    }
    if (m_rangeReply) {
        m_rangeReply->abort();
    }

    QNetworkReply *reply = m_network->getJson(this, this->pairUrl("/exchange/range", from, to).toString(),
                                              this->authHeaders());
    if (!reply) {
        emit rangeFailed("Offline mode is on");
        return;
    }

    m_rangeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();

        // Superseded by a newer request; nothing is waiting for this answer.
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return;
        }

        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            if (this->handledAsKeyFailure(reply)) {
                return;
            }
            emit rangeFailed(ChangeNowApi::errorText(reply, body));
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(body).object();
        if (obj.isEmpty()) {
            emit rangeFailed("The exchange sent a response that could not be read");
            return;
        }

        Range range;
        range.minAmount = ChangeNowApi::readAmount(obj, "minAmount");
        // Null means the pair has no ceiling, which is the usual answer here.
        const QJsonValue max = obj.value("maxAmount");
        const double maxAmount = ChangeNowApi::readAmount(obj, "maxAmount");
        range.hasMax = !max.isNull() && !max.isUndefined() && maxAmount > 0;
        range.maxAmount = range.hasMax ? maxAmount : 0;

        emit rangeReceived(range);
    });
}

void ChangeNowApi::requestQuote(const Asset &from, const Asset &to, double amount) {
    if (!ChangeNowApi::hasApiKey()) {
        QMetaObject::invokeMethod(this, &ChangeNowApi::keyRequired, Qt::QueuedConnection);
        return;
    }
    // Supersedes a reverse solve as well: both answer an edit that has since
    // been replaced.
    if (m_quoteReply) {
        m_quoteReply->abort();
    }
    if (m_reverseReply) {
        m_reverseReply->abort();
    }

    QNetworkReply *reply = this->sendEstimate(from, to, amount);
    if (!reply) {
        emit quoteFailed("Offline mode is on");
        return;
    }

    m_quoteReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, amount] {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError && this->handledAsKeyFailure(reply)) {
            return;
        }

        Quote quote;
        const QString error = ChangeNowApi::readEstimate(reply, amount, quote);
        if (!error.isEmpty()) {
            emit quoteFailed(error);
            return;
        }
        emit quoteReceived(quote);
    });
}

void ChangeNowApi::createOrder(const Asset &from, const Asset &to, double amount,
                               const QString &payoutAddress, const QString &refundAddress,
                               const QString &payoutExtraId) {
    if (!ChangeNowApi::hasApiKey()) {
        QMetaObject::invokeMethod(this, &ChangeNowApi::keyRequired, Qt::QueuedConnection);
        return;
    }
    if (m_orderReply) {
        m_orderReply->abort();
    }

    QJsonObject body;
    body["fromCurrency"] = from.ticker;
    body["fromNetwork"] = from.network;
    body["toCurrency"] = to.ticker;
    body["toNetwork"] = to.network;
    body["fromAmount"] = formatAmount(amount);
    body["address"] = payoutAddress;
    body["flow"] = kFlow;
    body["type"] = kType;
    if (!refundAddress.isEmpty()) {
        body["refundAddress"] = refundAddress;
    }
    if (!payoutExtraId.isEmpty()) {
        // Which of these two names the API reads could not be established from
        // outside -- its docs are not public and it silently ignores fields it
        // does not know, so a probe cannot tell the difference. Both are sent,
        // and the order panel then checks the tag actually came back registered
        // rather than assuming it did. Losing an exchange-bound XRP payout to a
        // dropped tag is not a failure worth guessing at.
        body["extraId"] = payoutExtraId;
        body["payoutExtraId"] = payoutExtraId;
    }

    QNetworkReply *reply = m_network->postJson(this, kApiBase + "/exchange", body, this->authHeaders());
    if (!reply) {
        emit orderFailed("Offline mode is on");
        return;
    }

    m_orderReply = reply;
    // By value: the reply outlives this call, and the create response does not
    // always name the pair back.
    const Asset fromAsset = from;
    const Asset toAsset = to;
    connect(reply, &QNetworkReply::finished, this, [this, reply, amount, fromAsset, toAsset] {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return;
        }

        const QByteArray replyBody = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            if (this->handledAsKeyFailure(reply)) {
                return;
            }
            emit orderFailed(ChangeNowApi::errorText(reply, replyBody));
            return;
        }

        Order order = ChangeNowApi::parseOrder(QJsonDocument::fromJson(replyBody).object());
        if (!order.isValid() || order.payinAddress.isEmpty()) {
            emit orderFailed("The exchange did not return a usable order");
            return;
        }

        // The create response echoes back neither a status nor, always, the
        // amounts, so both are filled in from what was asked for.
        if (order.status.isEmpty()) {
            order.status = QStringLiteral("new");
        }
        if (order.expectedFromAmount <= 0) {
            order.expectedFromAmount = amount;
        }
        order.fromCurrency = fromAsset.ticker.toUpper();
        order.toCurrency = toAsset.ticker.toUpper();
        order.fromNetwork = fromAsset.network;
        order.toNetwork = toAsset.network;

        emit orderCreated(order);
    });
}

void ChangeNowApi::requestOrder(const QString &id) {
    if (!ChangeNowApi::hasApiKey()) {
        QMetaObject::invokeMethod(this, &ChangeNowApi::keyRequired, Qt::QueuedConnection);
        return;
    }
    if (id.isEmpty()) {
        return;
    }

    if (m_statusReply) {
        m_statusReply->abort();
    }

    QUrlQuery query;
    query.addQueryItem("id", id);
    QUrl url{kApiBase + "/exchange/by-id"};
    url.setQuery(query);

    QNetworkReply *reply = m_network->getJson(this, url.toString(), this->authHeaders());
    if (!reply) {
        emit orderUpdateFailed("Offline mode is on");
        return;
    }

    m_statusReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return;
        }

        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            if (this->handledAsKeyFailure(reply)) {
                return;
            }
            emit orderUpdateFailed(ChangeNowApi::errorText(reply, body));
            return;
        }

        const Order order = ChangeNowApi::parseOrder(QJsonDocument::fromJson(body).object());
        if (!order.isValid()) {
            emit orderUpdateFailed("The exchange sent a response that could not be read");
            return;
        }

        emit orderUpdated(order);
    });
}

void ChangeNowApi::Order::completeFrom(const Order &previous) {
    if (id != previous.id) {
        return;
    }

    const auto keepText = [](QString &field, const QString &fallback) {
        if (field.isEmpty()) {
            field = fallback;
        }
    };
    const auto keepAmount = [](double &field, double fallback) {
        if (field <= 0) {
            field = fallback;
        }
    };

    keepAmount(expectedFromAmount, previous.expectedFromAmount);
    keepAmount(expectedToAmount, previous.expectedToAmount);
    keepText(payinAddress, previous.payinAddress);
    keepText(payoutAddress, previous.payoutAddress);
    keepText(refundAddress, previous.refundAddress);
    keepText(fromCurrency, previous.fromCurrency);
    keepText(toCurrency, previous.toCurrency);
    keepText(fromNetwork, previous.fromNetwork);
    keepText(toNetwork, previous.toNetwork);
    if (!validUntil.isValid()) {
        validUntil = previous.validUntil;
    }
}

ChangeNowApi::Order ChangeNowApi::parseOrder(const QJsonObject &obj) {
    Order order;
    order.id = obj.value("id").toString();
    order.status = obj.value("status").toString();
    order.payinAddress = obj.value("payinAddress").toString();
    order.payoutAddress = obj.value("payoutAddress").toString();
    order.refundAddress = obj.value("refundAddress").toString();
    order.fromCurrency = obj.value("fromCurrency").toString().toUpper();
    order.toCurrency = obj.value("toCurrency").toString().toUpper();
    order.fromNetwork = obj.value("fromNetwork").toString();
    order.toNetwork = obj.value("toNetwork").toString();
    order.payinHash = obj.value("payinHash").toString();
    order.payoutExtraId = obj.value("payoutExtraId").toString();
    order.payoutHash = obj.value("payoutHash").toString();
    order.validUntil = QDateTime::fromString(obj.value("validUntil").toString(), Qt::ISODate);

    // Create and status endpoints name the amounts differently, and a settled
    // swap carries what actually moved; prefer that over the estimate.
    const auto firstAmount = [&obj](std::initializer_list<const char *> keys) {
        for (const char *key : keys) {
            const double amount = ChangeNowApi::readAmount(obj, QLatin1String(key));
            if (amount > 0) {
                return amount;
            }
        }
        return 0.0;
    };

    order.expectedFromAmount = firstAmount({"amountFrom", "expectedAmountFrom", "fromAmount"});
    order.expectedToAmount = firstAmount({"amountTo", "expectedAmountTo", "toAmount"});

    return order;
}

QString ChangeNowApi::trackingUrl(const QString &id) {
    return QString("https://changenow.io/exchange/txs/%1").arg(QString(QUrl::toPercentEncoding(id)));
}

bool ChangeNowApi::isFinalStatus(const QString &status) {
    const QString s = status.toLower();
    return s == "finished" || s == "failed" || s == "refunded" || s == "expired";
}

bool ChangeNowApi::isInProgress(const QString &status) {
    const QString s = status.toLower();
    return s == "confirming" || s == "exchanging" || s == "sending" || s == "verifying";
}

QString ChangeNowApi::statusText(const QString &status) {
    const QString s = status.toLower();
    if (s == "new")        return "Waiting for your deposit";
    if (s == "waiting")    return "Waiting for your deposit";
    if (s == "confirming") return "Confirming your deposit";
    if (s == "exchanging") return "Exchanging";
    if (s == "sending")    return "Sending your funds";
    if (s == "finished")   return "Completed";
    if (s == "failed")     return "Failed";
    if (s == "refunded")   return "Refunded";
    if (s == "verifying")  return "Being verified by the exchange";
    if (s == "expired")    return "Expired, no deposit received";
    return status.isEmpty() ? QString("Unknown") : status;
}

QString ChangeNowApi::errorText(QNetworkReply *reply, const QByteArray &body) {
    const QJsonObject obj = QJsonDocument::fromJson(body).object();

    // "message" is the sentence meant for a human; "error" is the machine code.
    const QString message = obj.value("message").toString();
    const QString error = obj.value("error").toString();

    // The message names the address but never which field it came from, and the
    // two fields want addresses on different chains. Only the code says which.
    if (!message.isEmpty()) {
        if (error == "not_valid_refund_address") {
            return QString("Refund address: %1").arg(message);
        }
        if (error == "not_valid_address") {
            return QString("Receiving address: %1").arg(message);
        }
        return message;
    }

    if (!error.isEmpty()) {
        return error;
    }

    // Non-JSON body: a proxy refusing the connection, a captive portal.
    return reply->errorString();
}

QNetworkReply *ChangeNowApi::sendEstimate(const Asset &from, const Asset &to, double amount) {
    QUrl url = this->pairUrl("/exchange/estimated-amount", from, to);
    QUrlQuery query{url.query()};
    query.addQueryItem("fromAmount", formatAmount(amount));
    query.addQueryItem("type", kType);
    url.setQuery(query);
    return m_network->getJson(this, url.toString(), this->authHeaders());
}

QString ChangeNowApi::readEstimate(QNetworkReply *reply, double amount, Quote &quote) {
    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        return ChangeNowApi::errorText(reply, body);
    }

    const QJsonObject obj = QJsonDocument::fromJson(body).object();
    if (obj.isEmpty()) {
        return QStringLiteral("The exchange sent a response that could not be read");
    }

    quote.fromAmount = ChangeNowApi::readAmount(obj, "fromAmount");
    if (quote.fromAmount <= 0) {
        quote.fromAmount = amount;
    }
    quote.toAmount = ChangeNowApi::readAmount(obj, "toAmount");
    quote.rate = quote.fromAmount > 0 ? quote.toAmount / quote.fromAmount : 0;
    quote.warning = obj.value("warningMessage").toString();
    quote.speedForecast = obj.value("transactionSpeedForecast").toString();
    quote.withdrawalFee = ChangeNowApi::readAmount(obj, "withdrawalFee");

    if (quote.toAmount <= 0) {
        // A zero payout for a non-zero deposit: the pair cannot be quoted at
        // this size, and "0" would read as a real rate.
        return QStringLiteral("The exchange could not quote this amount");
    }
    return {};
}

void ChangeNowApi::requestReverseQuote(const Asset &from, const Asset &to, double targetTo,
                                       double seedFrom, double minFrom) {
    if (!ChangeNowApi::hasApiKey()) {
        QMetaObject::invokeMethod(this, &ChangeNowApi::keyRequired, Qt::QueuedConnection);
        return;
    }
    if (m_reverseReply) {
        m_reverseReply->abort();
    }
    if (m_quoteReply) {
        m_quoteReply->abort();
    }

    m_reverse = ReverseSolve{};
    m_reverse.from = from;
    m_reverse.to = to;
    m_reverse.target = targetTo;
    m_reverse.minFrom = roundAmount(qMax(0.0, minFrom));

    if (targetTo <= 0) {
        emit reverseQuoteFailed(QStringLiteral("Enter an amount to receive"));
        return;
    }

    this->reverseStep(qMax(seedFrom, m_reverse.minFrom));
}

void ChangeNowApi::reverseStep(double fromAmount) {
    // Priced at exactly the precision it will be sent at, so the deposit filled
    // in is the deposit that was quoted.
    fromAmount = roundAmount(fromAmount);
    if (fromAmount <= 0) {
        emit reverseQuoteFailed(QStringLiteral("The exchange could not quote this amount"));
        return;
    }

    QNetworkReply *reply = this->sendEstimate(m_reverse.from, m_reverse.to, fromAmount);
    if (!reply) {
        emit reverseQuoteFailed(QStringLiteral("Offline mode is on"));
        return;
    }

    m_reverseReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, fromAmount] {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError && this->handledAsKeyFailure(reply)) {
            return;
        }

        Quote quote;
        const QString error = ChangeNowApi::readEstimate(reply, fromAmount, quote);
        if (!error.isEmpty()) {
            emit reverseQuoteFailed(error);
            return;
        }

        ReverseSolve &solve = m_reverse;
        solve.points.append({fromAmount, quote.toAmount});

        const double miss = qAbs(quote.toAmount - solve.target) / solve.target;
        if (solve.bestMiss < 0 || miss < solve.bestMiss) {
            solve.bestMiss = miss;
            solve.bestFrom = fromAmount;
            solve.bestQuote = quote;
        }
        if (solve.minFrom > 0 && qAbs(fromAmount - solve.minFrom) < 1e-12) {
            solve.quotedMinimum = true;
            solve.minimumQuote = quote;
        }

        if (miss <= kReverseTolerance) {
            emit reverseQuoteReceived(fromAmount, quote, false);
            return;
        }

        const double next = roundAmount(ChangeNowApi::nextReverseGuess(solve.points, solve.target));

        // Payouts rise with the deposit, so the target is under the minimum
        // exactly when the minimum itself pays more than the target. That is
        // settled by quoting the minimum once, not by trusting an extrapolation
        // that may simply have overshot.
        if (solve.minFrom > 0 && next < solve.minFrom) {
            if (solve.quotedMinimum) {
                emit reverseQuoteReceived(next > 0 ? next : solve.minFrom, solve.minimumQuote, true);
                return;
            }
            this->reverseStep(solve.minFrom);
            return;
        }

        if (next <= 0 || next == fromAmount || solve.points.size() >= kReverseMaxQuotes) {
            // Converged to the precision amounts are sent at, or out of budget:
            // the closest point priced is the answer.
            emit reverseQuoteReceived(solve.bestFrom, solve.bestQuote, false);
            return;
        }

        this->reverseStep(next);
    });
}

double ChangeNowApi::nextReverseGuess(const QList<QPair<double, double>> &points, double targetTo) {
    if (points.isEmpty() || targetTo <= 0) {
        return 0;
    }

    const double x1 = points.last().first;
    const double y1 = points.last().second;

    // The newest point against whichever earlier one lies farthest from it,
    // rather than the last two. Two points that have converged on each other
    // differ by little more than quote jitter, so a secant through them is
    // mostly noise; a wide baseline keeps the slope honest. Simulated with
    // live-sized jitter, this matches a plain secant while the rate holds still
    // and lands within tolerance slightly more often when it moves between
    // quotes -- which it can, over a Tor round-trip.
    int far = -1;
    for (int i = 0; i + 1 < points.size(); ++i) {
        if (far < 0 || qAbs(points.at(i).first - x1) > qAbs(points.at(far).first - x1)) {
            far = i;
        }
    }

    if (far >= 0) {
        const double x0 = points.at(far).first;
        const double y0 = points.at(far).second;
        if (qAbs(x1 - x0) >= kMinBaseline * x1 && y1 != y0) {
            const double next = x1 + (targetTo - y1) * (x1 - x0) / (y1 - y0);
            if (next > 0) {
                return next;
            }
        }
    }

    // One point, or none far enough away to trust: scale by the ratio. That
    // ignores the fee, but near the answer what it leaves is a small fraction
    // of an already small miss.
    return y1 > 0 ? x1 * targetTo / y1 : 0;
}
