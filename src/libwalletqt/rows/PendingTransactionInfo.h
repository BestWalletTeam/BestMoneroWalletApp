// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_PENDINGTRANSACTIONINFO_H
#define BESTWALLET_PENDINGTRANSACTIONINFO_H

#include "ConstructionInfo.h"

#include <QString>

namespace Monero {
    class PendingTransactionInfo;
}

struct PendingTransactionInfo : ConstructionInfo
{
    quint64 fee;
    quint64 dust;
    bool dustAddedToFee;
    QString txKey;

    explicit PendingTransactionInfo(const Monero::PendingTransactionInfo *pimpl);
};

#endif //BESTWALLET_PENDINGTRANSACTIONINFO_H
