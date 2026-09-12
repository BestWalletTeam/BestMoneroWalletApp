// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_INPUT_H
#define BESTWALLET_INPUT_H

struct Input
{
    QString pubKey;
    quint64 amount;

    explicit Input(uint64_t amount, QString pubkey)
        : pubKey(std::move(pubkey))
        , amount(amount) {}
};

#endif //BESTWALLET_INPUT_H
