// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_CONSTRUCTIONINFO_H
#define BESTWALLET_CONSTRUCTIONINFO_H

#include <QSet>

#include "Output.h"
#include "Input.h"

namespace Monero {
    class TransactionConstructionInfo;
}

struct ConstructionInfo
{
    quint64 unlockTime;
    QSet<quint32> subaddressIndices;
    QVector<QString> subaddresses;
    quint64 minMixinCount;
    QList<Input> inputs;
    QList<Output> outputs;

    explicit ConstructionInfo(const Monero::TransactionConstructionInfo *pimpl);
};

#endif //BESTWALLET_CONSTRUCTIONINFO_H
