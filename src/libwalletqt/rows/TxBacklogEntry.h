// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_TXBACKLOGENTRY_H
#define BESTWALLET_TXBACKLOGENTRY_H

struct TxBacklogEntry {
    quint64 weight;
    quint64 fee;
    quint64 timeInPool;
};

#endif //BESTWALLET_TXBACKLOGENTRY_H
