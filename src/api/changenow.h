// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_CHANGENOW_H
#define BESTWALLET_CHANGENOW_H

#include <QDateTime>
#include <QObject>
#include <QList>
#include <QPair>
#include <QPointer>

#include "utils/Networking.h"

class QJsonObject;
class QNetworkReply;

// Client for the ChangeNOW instant-exchange API (v2). Every endpoint
// authenticates with an x-changenow-api-key header; see apiKey().
class ChangeNowApi : public QObject
{
    Q_OBJECT

public:
    // An asset as the API addresses it. Ticker and network are carried together
    // because a token exists on several chains at once, and USDT on Tron is a
    // different asset from USDT on Ethereum.
    struct Asset {
        QString ticker;   // "btc", "usdt"
        QString network;  // "btc", "trx"

        // Whether a payout to this asset can carry a destination tag / memo.
        bool hasMemo = false;

        bool isValid() const { return !ticker.isEmpty() && !network.isEmpty(); }
        bool operator==(const Asset &o) const { return ticker == o.ticker && network == o.network; }
    };

    struct Quote {
        double fromAmount = 0;
        double toAmount = 0;
        double rate = 0;
        // Advisory text the API attaches to an otherwise valid estimate.
        QString warning;

        // What the exchange deducts to send the payout, in the payout coin.
        double withdrawalFee = 0;

        // Expected duration as a range of minutes ("10-60"). Only the quote
        // endpoints report it, not an order looked up by id.
        QString speedForecast;
    };

    struct Range {
        double minAmount = 0;
        // A null maxAmount from the API means no ceiling, and arrives here as
        // 0, so callers must check hasMax before enforcing an upper bound.
        double maxAmount = 0;
        bool hasMax = false;
    };

    struct Order {
        QString id;
        QString status;
        QString payinAddress;
        QString payoutAddress;
        QString refundAddress;
        QString fromCurrency;
        QString toCurrency;
        // Carried alongside the tickers so a restored order still knows which
        // chain it is on.
        QString fromNetwork;
        QString toNetwork;
        double expectedFromAmount = 0;
        double expectedToAmount = 0;
        QString payinHash;
        QString payoutHash;

        // The tag the exchange registered, read back so the UI can prove it
        // took rather than trusting it was accepted.
        QString payoutExtraId;
        QDateTime validUntil;

        bool isValid() const { return !id.isEmpty(); }

        // Fills the gaps in this record from an earlier one for the same order.
        // The status endpoint answers about progress and leaves the fields it
        // does not speak to empty or zero, which would otherwise wipe details
        // established at creation on the first poll.
        void completeFrom(const Order &previous);
    };

    explicit ChangeNowApi(QObject *parent = nullptr);

    // The accepted deposit range for a pair, which depends only on the pair.
    void requestRange(const Asset &from, const Asset &to);

    // Estimates the payout for `amount`. Only one estimate is in flight at a
    // time: a new request aborts the previous one, so an older reply cannot
    // land after a newer one.
    void requestQuote(const Asset &from, const Asset &to, double amount);

    // Works out the deposit that pays out `targetTo`. The floating flow only
    // prices the direct direction -- "type reverse" is refused there -- so this
    // solves for it with direct quotes, usually two to four. Requests are
    // never made below `minFrom`; `seedFrom` is a first guess.
    void requestReverseQuote(const Asset &from, const Asset &to, double targetTo,
                             double seedFrom, double minFrom);

    // One step of that solve, given the (deposit, payout) points quoted so far.
    // Pure, so it can be tested without the network.
    static double nextReverseGuess(const QList<QPair<double, double>> &points, double targetTo);

    // Opens an order. An empty `refundAddress` leaves the exchange nowhere to
    // return a deposit it cannot process.
    void createOrder(const Asset &from, const Asset &to, double amount,
                     const QString &payoutAddress, const QString &refundAddress,
                     const QString &payoutExtraId = {});

    // Refreshes an order's status.
    void requestOrder(const QString &id);

    static bool hasApiKey();

    // The user's key if one was put in the config, otherwise the bundled one.
    static QString apiKey();

    // Read from Config::swapApiKey, which nothing in the UI writes: it is the
    // way to rotate a refused key by hand without shipping a build.
    static QString userApiKey();
    static bool usingBundledKey();

    // The exchange's public page for an order. Orders the wallet creates are
    // not attached to any account, so this is the only way to follow one
    // outside the wallet.
    static QString trackingUrl(const QString &id);

    // True once the order can no longer change. Polling stops here.
    static bool isFinalStatus(const QString &status);

    // Human-readable form of the API's status vocabulary.
    static QString statusText(const QString &status);

    // True when the exchange is holding funds, so abandoning the order is worth
    // warning about.
    static bool isInProgress(const QString &status);

signals:
    void rangeReceived(const ChangeNowApi::Range &range);
    void rangeFailed(const QString &error);

    void quoteReceived(const ChangeNowApi::Quote &quote);
    void quoteFailed(const QString &error);

    // With `belowMinimum`, the deposit that would pay the target is under what
    // the exchange accepts: `fromAmount` is extrapolated, not quoted, and
    // `quote` is the one for the minimum.
    void reverseQuoteReceived(double fromAmount, const ChangeNowApi::Quote &quote, bool belowMinimum);
    void reverseQuoteFailed(const QString &error);

    void orderCreated(const ChangeNowApi::Order &order);
    void orderFailed(const QString &error);

    void orderUpdated(const ChangeNowApi::Order &order);
    void orderUpdateFailed(const QString &error);

    // Emitted when there is no key, or the exchange refused the one sent.
    void keyRequired();

private:
    Networking::Headers authHeaders() const;

    // Shared setup for the two quote endpoints, which differ only in path.
    QUrl pairUrl(const QString &path, const Asset &from, const Asset &to) const;

    // Returns true and emits keyRequired() when the reply was a 401.
    bool handledAsKeyFailure(QNetworkReply *reply);

    QNetworkReply *sendEstimate(const Asset &from, const Asset &to, double amount);

    // Reads an estimate reply into `quote`. Returns the error, or empty.
    static QString readEstimate(QNetworkReply *reply, double amount, Quote &quote);

    void reverseStep(double fromAmount);

    // Pulls the API's own error text out of a failed reply, falling back for
    // bodies that are not JSON at all.
    static QString errorText(QNetworkReply *reply, const QByteArray &body);

    static Order parseOrder(const QJsonObject &obj);

    // Amounts arrive as either a number or a string depending on the endpoint.
    static double readAmount(const QJsonObject &obj, const QString &key);

    Networking *m_network;
    QPointer<QNetworkReply> m_rangeReply;
    QPointer<QNetworkReply> m_quoteReply;
    QPointer<QNetworkReply> m_reverseReply;

    struct ReverseSolve {
        Asset from;
        Asset to;
        double target = 0;
        double minFrom = 0;
        QList<QPair<double, double>> points;
        double bestFrom = 0;
        double bestMiss = -1;
        Quote bestQuote;
        bool quotedMinimum = false;
        Quote minimumQuote;
    };
    ReverseSolve m_reverse;
    QPointer<QNetworkReply> m_orderReply;
    QPointer<QNetworkReply> m_statusReply;
};

Q_DECLARE_METATYPE(ChangeNowApi::Quote)
Q_DECLARE_METATYPE(ChangeNowApi::Range)
Q_DECLARE_METATYPE(ChangeNowApi::Order)

#endif //BESTWALLET_CHANGENOW_H
