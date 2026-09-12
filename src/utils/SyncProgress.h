// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SYNCPROGRESS_H
#define BESTWALLET_SYNCPROGRESS_H

#include <deque>

#include <QElapsedTimer>
#include <QString>

// Turns a stream of (height, target) sync updates into a percentage and a time
// estimate. Updates arrive once per scanned block and scanning is bursty:
// wallet2 pulls a chunk of blocks, waits on the network, then tears through the
// chunk. The rate is therefore measured over a trailing window wide enough to
// span several of those cycles, never over a single update.
class SyncProgress
{
public:
    // A daemonSync update counts the node's blocks rather than the wallet's, at
    // an unrelated rate; the two are never averaged together.
    void update(quint64 height, quint64 target, bool daemonSync = false);

    bool isSyncing() const { return m_syncing; }

    // 0-100, reporting 100 only when actually caught up rather than as a
    // rounding of 99.6.
    int percent() const;

    quint64 remaining() const { return m_target > m_height ? m_target - m_height : 0; }

    // False until enough has been seen to say anything honest.
    bool hasEstimate() const;

    // "about 4 minutes left", or empty when there is no estimate worth giving.
    QString estimateText() const;

    // "2,788,652 of 3,201,455 blocks · 87%", or empty when not syncing. Counts
    // chain position rather than blocks fetched this session, so it does not
    // jump when the wallet is reopened part-way through.
    QString progressText() const;

private:
    // The far end of the measuring window: a height and when it was seen.
    struct Anchor {
        qint64 ms;
        quint64 height;
    };

    void forget();

    // Blocks per second across the window, or 0 while it is too short to say.
    double rate() const;

    // Negative when there is nothing to go on.
    qint64 secondsLeft() const;

    std::deque<Anchor> m_anchors;
    QElapsedTimer m_clock;
    qint64 m_now = 0;

    quint64 m_height = 0;
    quint64 m_target = 0;
    bool m_syncing = false;
    bool m_daemonSync = false;
};

#endif //BESTWALLET_SYNCPROGRESS_H
