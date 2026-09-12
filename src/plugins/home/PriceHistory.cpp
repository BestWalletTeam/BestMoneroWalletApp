// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "PriceHistory.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

#include <algorithm>

#include "utils/config.h"
#include "utils/Networking.h"
#include "utils/TorManager.h"

namespace {
    const QVector<PriceHistory::Range> kAllRanges = {
        PriceHistory::Range::Day,
        PriceHistory::Range::Week,
        PriceHistory::Range::Month,
        PriceHistory::Range::Year,
    };

    // Capped, so a wallet left open behind a dead network still checks back.
    int backoffSeconds(int failures)
    {
        static const int ladder[] = {15, 30, 60, 120, 300};
        constexpr int last = int(sizeof(ladder) / sizeof(ladder[0])) - 1;
        return ladder[qBound(0, failures - 1, last)];
    }
}

PriceHistory::PriceHistory(QObject *parent)
    : QObject(parent)
{
    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, &PriceHistory::retryPending);

    this->load();
}

QVector<QPointF> PriceHistory::series(Range range)
{
    Entry &entry = m_entries[int(range)];
    entry.wanted = true;

    this->maybeFetch(range);
    return entry.points;
}

bool PriceHistory::isFetching(Range range) const
{
    return m_inFlight.contains(int(range));
}

bool PriceHistory::hasAnyData() const
{
    return !m_lookup.isEmpty();
}

double PriceHistory::spot() const
{
    return m_lookup.isEmpty() ? 0.0 : m_lookup.last().y();
}

QVector<QPointF> PriceHistory::pricesBetween(double fromMs, double toMs) const
{
    QVector<QPointF> out;
    if (m_lookup.isEmpty() || toMs <= fromMs) {
        return out;
    }

    const auto begin = std::lower_bound(
        m_lookup.constBegin(), m_lookup.constEnd(), fromMs,
        [](const QPointF &point, double value) { return point.x() < value; });

    for (auto it = begin; it != m_lookup.constEnd() && it->x() <= toMs; ++it) {
        out.append(*it);
    }
    return out;
}

double PriceHistory::priceAt(double msSinceEpoch) const
{
    if (m_lookup.isEmpty()) {
        return 0.0;
    }

    // Nearest sample at or before the moment asked for, clamped at both ends.
    const auto it = std::upper_bound(
        m_lookup.constBegin(), m_lookup.constEnd(), msSinceEpoch,
        [](double value, const QPointF &point) { return value < point.x(); });

    if (it == m_lookup.constBegin()) {
        return m_lookup.first().y();
    }
    return (it - 1)->y();
}

// ------------------------------- fetching -------------------------------

bool PriceHistory::transportReady() const
{
    if (conf()->get(Config::offlineMode).toBool()) {
        return false;
    }

    const int proxy = conf()->get(Config::proxy).toInt();
    if (proxy == Config::Proxy::None) {
        return true;
    }
    if (proxy == Config::Proxy::Tor) {
        // Means "finished bootstrapping", not merely "the SOCKS port is open".
        return torManager()->torConnected;
    }

    // A user-supplied proxy cannot be probed from here; the backoff handles it.
    return true;
}

void PriceHistory::maybeFetch(Range range)
{
    const int key = int(range);
    if (m_inFlight.contains(key)) {
        return;
    }

    Entry &entry = m_entries[key];
    const QDateTime now = QDateTime::currentDateTime();

    const bool haveData = !entry.points.isEmpty();
    if (haveData && entry.fetched.isValid()
        && entry.fetched.secsTo(now) < rangeTtlSeconds(range)) {
        return;                                 // cached copy is still fresh
    }

    if (entry.attempted.isValid()) {
        const qint64 since = entry.attempted.secsTo(now);
        const qint64 wait = entry.failures > 0 ? backoffSeconds(entry.failures) : 30;
        if (since < wait) {
            this->scheduleRetry(int(wait - since));
            return;
        }
    }

    if (!this->transportReady()) {
        const QString reason = conf()->get(Config::offlineMode).toBool()
            ? "Offline mode is on"
            : "Waiting for Tor to connect";

        this->scheduleRetry(10);

        // Only on a change: updated() redraws the chart, which asks for the
        // series again and lands back here.
        if (m_status != reason) {
            m_status = reason;
            emit updated();
        }
        return;
    }

    this->fetch(range);
}

void PriceHistory::fetch(Range range)
{
    const int key = int(range);
    m_entries[key].attempted = QDateTime::currentDateTime();

    if (!m_network) {
        m_network = new Networking(this);
    }

    const QString url = QString("https://api.coingecko.com/api/v3/coins/monero/market_chart"
                                "?vs_currency=usd&days=%1").arg(rangeDays(range));

    QNetworkReply *reply = m_network->getJson(this, url);
    if (!reply) {
        this->onFailure(range, "Offline mode is on");
        return;
    }

    m_inFlight.insert(key, reply);
    connect(reply, &QNetworkReply::finished, this, [this, range, reply] {
        this->onReply(range, reply);
    });
}

void PriceHistory::onReply(Range range, QNetworkReply *reply)
{
    m_inFlight.remove(int(range));
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        this->onFailure(range, reply->errorString());
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        this->onFailure(range, "The price response could not be read");
        return;
    }

    const QJsonArray prices = doc.object().value("prices").toArray();
    QVector<QPointF> points;
    points.reserve(prices.size());
    for (const auto &item : prices) {
        // Each entry is [milliseconds, price].
        const QJsonArray pair = item.toArray();
        if (pair.size() < 2) {
            continue;
        }
        const double stamp = pair.at(0).toDouble();
        const double price = pair.at(1).toDouble();
        if (stamp <= 0 || price <= 0) {
            continue;
        }
        points.append(QPointF(stamp, price));
    }

    if (points.isEmpty()) {
        this->onFailure(range, "No price data was returned");
        return;
    }

    Entry &entry = m_entries[int(range)];
    entry.points = points;
    entry.fetched = QDateTime::currentDateTime();
    entry.failures = 0;
    m_status.clear();

    this->rebuildLookup();
    this->save();

    emit updated();
}

void PriceHistory::onFailure(Range range, const QString &why)
{
    Entry &entry = m_entries[int(range)];
    entry.failures += 1;
    m_status = why;

    this->scheduleRetry(backoffSeconds(entry.failures));

    // Anything cached stays on screen; this only moves the empty state off
    // "loading" for a range that was never fetched.
    emit updated();
}

void PriceHistory::scheduleRetry(int seconds)
{
    const int ms = qMax(1, seconds) * 1000;
    if (m_retryTimer.isActive() && m_retryTimer.remainingTime() <= ms) {
        return;                                 // an earlier wake-up is enough
    }
    m_retryTimer.start(ms);
}

void PriceHistory::retryPending()
{
    for (const Range range : kAllRanges) {
        if (m_entries.value(int(range)).wanted) {
            this->maybeFetch(range);
        }
    }
}

// ------------------------------ persistence ------------------------------

QString PriceHistory::cachePath()
{
    return Config::defaultConfigDir().filePath("pricehistory.json");
}

void PriceHistory::load()
{
    QFile file{cachePath()};
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object().value("usd").toObject();
    for (const Range range : kAllRanges) {
        const QJsonObject obj = root.value(rangeKey(range)).toObject();
        if (obj.isEmpty()) {
            continue;
        }

        QVector<QPointF> points;
        const QJsonArray arr = obj.value("points").toArray();
        points.reserve(arr.size());
        for (const auto &item : arr) {
            const QJsonArray pair = item.toArray();
            if (pair.size() < 2) {
                continue;
            }
            const double stamp = pair.at(0).toDouble();
            const double price = pair.at(1).toDouble();
            if (stamp > 0 && price > 0) {
                points.append(QPointF(stamp, price));
            }
        }
        if (points.isEmpty()) {
            continue;
        }

        Entry &entry = m_entries[int(range)];
        entry.points = points;
        entry.fetched = QDateTime::fromMSecsSinceEpoch(
            qint64(obj.value("fetched").toDouble()));
    }

    this->rebuildLookup();
}

void PriceHistory::save() const
{
    QJsonObject root;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        const Entry &entry = it.value();
        if (entry.points.isEmpty()) {
            continue;
        }

        QJsonArray points;
        for (const QPointF &point : entry.points) {
            points.append(QJsonArray{point.x(), point.y()});
        }

        QJsonObject obj;
        obj.insert("fetched", double(entry.fetched.toMSecsSinceEpoch()));
        obj.insert("points", points);
        root.insert(rangeKey(Range(it.key())), obj);
    }

    QJsonObject doc;
    doc.insert("usd", root);

    const QString path = cachePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file{path};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Could not write price history cache:" << path;
        return;
    }
    file.write(QJsonDocument(doc).toJson(QJsonDocument::Compact));
    file.close();
}

void PriceHistory::rebuildLookup()
{
    QVector<QPointF> merged;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        merged += it.value().points;
    }

    std::sort(merged.begin(), merged.end(),
              [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

    m_lookup = merged;
}

// -------------------------------- ranges --------------------------------

QString PriceHistory::rangeKey(Range range)
{
    switch (range) {
        case Range::Day:   return "1";
        case Range::Week:  return "7";
        case Range::Month: return "30";
        case Range::Year:  return "365";
    }
    return "30";
}

QString PriceHistory::rangeDays(Range range)
{
    // CoinGecko picks granularity from the span: 5-minutely under a day,
    // hourly up to 90 days, daily beyond.
    return rangeKey(range);
}

qint64 PriceHistory::rangeTtlSeconds(Range range)
{
    // Long spans are drawn from coarse samples, so they go stale more slowly.
    switch (range) {
        case Range::Day:   return 10 * 60;
        case Range::Week:  return 30 * 60;
        case Range::Month: return 2 * 60 * 60;
        case Range::Year:  return 12 * 60 * 60;
    }
    return 60 * 60;
}
