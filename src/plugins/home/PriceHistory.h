// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_PRICEHISTORY_H
#define BESTWALLET_PRICEHISTORY_H

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QPointF>
#include <QTimer>
#include <QVector>

class Networking;
class QNetworkReply;

// XMR price history from CoinGecko, always in USD.
//
// Every series is written to disk and served from there immediately; the network
// only refreshes a copy that has gone stale, and a failed refresh leaves the
// cached copy in place.
class PriceHistory : public QObject
{
    Q_OBJECT

public:
    // The free public API refuses anything older than a year.
    enum class Range { Day, Week, Month, Year };

    explicit PriceHistory(QObject *parent = nullptr);

    // Cached USD series for a range, oldest first. Marks the range as wanted and
    // refreshes it if stale.
    QVector<QPointF> series(Range range);

    // USD price at a moment, or 0 when nothing is cached.
    double priceAt(double msSinceEpoch) const;

    // Most recent cached USD price, or 0 when there is none.
    double spot() const;

    // Cached samples inside a window, oldest first.
    QVector<QPointF> pricesBetween(double fromMs, double toMs) const;

    bool isFetching(Range range) const;
    bool hasAnyData() const;

    // Why there is nothing to show yet, for the chart's empty state. Empty once
    // a fetch has succeeded.
    QString status() const { return m_status; }

signals:
    void updated();

private:
    struct Entry {
        QVector<QPointF> points;    // [epoch ms, USD]
        QDateTime fetched;          // last successful fetch
        QDateTime attempted;        // last attempt, successful or not
        int failures = 0;
        bool wanted = false;        // something has asked for this range
    };

    void maybeFetch(Range range);
    void fetch(Range range);
    void onReply(Range range, QNetworkReply *reply);
    void onFailure(Range range, const QString &why);
    void retryPending();
    void scheduleRetry(int seconds);

    // Whether a request stands any chance right now.
    bool transportReady() const;

    void load();
    void save() const;
    static QString cachePath();
    static QString rangeKey(Range range);
    static QString rangeDays(Range range);
    static qint64 rangeTtlSeconds(Range range);

    Networking *m_network = nullptr;
    QHash<int, Entry> m_entries;
    QHash<int, QNetworkReply*> m_inFlight;
    QTimer m_retryTimer;
    QString m_status;

    // Every cached point, sorted by time, so priceAt() can look across ranges.
    void rebuildLookup();
    QVector<QPointF> m_lookup;
};

#endif //BESTWALLET_PRICEHISTORY_H
