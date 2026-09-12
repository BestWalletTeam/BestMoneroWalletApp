// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_ACCOUNTROW_H
#define BESTWALLET_ACCOUNTROW_H

#include <QString>

struct AccountRow
{
    QString address;
    QString label;
    quint64 balance;
    quint64 unlockedBalance;

    AccountRow(const QString& address, const QString &label, uint64_t balance, uint64_t unlockedBalance)
            : address(address)
            , label(label)
            , balance(balance)
            , unlockedBalance(unlockedBalance) {}
};

#endif //BESTWALLET_ACCOUNTROW_H
