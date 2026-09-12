// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "SyncProgress.h"

#include <QLocale>

namespace {
    // How far back the window reaches, and how finely it is anchored. Twenty
    // seconds covers several fetch-and-scan cycles, so a wallet that happens to
    // be waiting on the network is not mistaken for a slow one.
    constexpr qint64 kWindowMs = 20 * 1000;
    constexpr qint64 kAnchorIntervalMs = 1000;

    // Below this the window says more about which part of a burst it caught
    // than about the sync.
    constexpr qint64 kMinWindowMs = 5 * 1000;

    // Beyond this the estimate is not worth showing.
    constexpr qint64 kMaxSensibleSeconds = 24 * 60 * 60;
}

void SyncProgress::forget() {
    m_anchors.clear();
}

void SyncProgress::update(quint64 height, quint64 target, bool daemonSync) {
    // The chain tip moves under you, so the last block is never quite reached;
    // this mirrors how the rest of the wallet decides it is caught up.
    if (target == 0 || height + 1 >= target) {
        this->forget();
        m_height = height;
        m_target = target;
        m_syncing = false;
        return;
    }

    // A switch between the two kinds of sync, a different chain, or a wallet
    // rescanned from an earlier height: what was measured before it says
    // nothing about what follows.
    const bool restarted = daemonSync != m_daemonSync || height < m_height
                           || (m_target != 0 && target < m_target);
    if (restarted) {
        this->forget();
    }

    if (!m_clock.isValid()) {
        m_clock.start();
    }
    m_now = m_clock.elapsed();

    // Anchors only hold the far end of the window open. The near end is always
    // the current height, so every block scanned counts towards the rate even
    // though a new anchor is laid down just once a second.
    if (m_anchors.empty() || m_now - m_anchors.back().ms >= kAnchorIntervalMs) {
        m_anchors.push_back({m_now, height});
    }
    while (m_anchors.size() > 1 && m_now - m_anchors.front().ms > kWindowMs) {
        m_anchors.pop_front();
    }

    m_height = height;
    m_target = target;
    m_daemonSync = daemonSync;
    m_syncing = true;
}

int SyncProgress::percent() const {
    if (m_target == 0) {
        return 0;
    }
    if (!m_syncing) {
        return 100;
    }
    const double fraction = double(m_height) / double(m_target);
    // Floored, so the bar cannot show a full 100 while blocks remain.
    const int pct = int(fraction * 100.0);
    return qBound(0, pct, 99);
}

double SyncProgress::rate() const {
    if (m_anchors.empty()) {
        return 0;
    }

    const Anchor &oldest = m_anchors.front();
    const qint64 span = m_now - oldest.ms;
    if (span < kMinWindowMs || m_height <= oldest.height) {
        return 0;
    }

    return double(m_height - oldest.height) * 1000.0 / double(span);
}

qint64 SyncProgress::secondsLeft() const {
    const double blocksPerSecond = this->rate();
    if (blocksPerSecond <= 0) {
        return -1;
    }
    return qint64(double(this->remaining()) / blocksPerSecond);
}

bool SyncProgress::hasEstimate() const {
    const qint64 seconds = this->secondsLeft();
    return m_syncing && seconds >= 0 && seconds <= kMaxSensibleSeconds;
}

QString SyncProgress::estimateText() const {
    if (!this->hasEstimate()) {
        return {};
    }

    const qint64 seconds = this->secondsLeft();
    if (seconds < 60) {
        return QStringLiteral("less than a minute left");
    }

    // Rounded more coarsely the further out it goes: the estimate is nowhere
    // near precise enough to justify a figure that twitches every block.
    qint64 minutes = (seconds + 30) / 60;
    if (minutes < 60) {
        const qint64 step = minutes < 10 ? 1 : 5;
        minutes = qMax(step, (minutes + step / 2) / step * step);
        return QStringLiteral("about %1 minute%2 left").arg(minutes).arg(minutes == 1 ? "" : "s");
    }

    minutes = (minutes + 7) / 15 * 15;
    const qint64 hours = minutes / 60;
    const qint64 rest = minutes % 60;
    if (rest == 0) {
        return QStringLiteral("about %1 hour%2 left").arg(hours).arg(hours == 1 ? "" : "s");
    }
    return QStringLiteral("about %1 h %2 m left").arg(hours).arg(rest);
}

QString SyncProgress::progressText() const {
    if (!m_syncing || m_target == 0) {
        return {};
    }

    // Grouped digits: these run to seven figures, and an ungrouped 3201455 has
    // to be counted rather than read.
    const QLocale locale;
    return QString("%1 of %2 blocks · %3%")
        .arg(locale.toString(qulonglong(m_height)),
             locale.toString(qulonglong(m_target)))
        .arg(this->percent());
}
