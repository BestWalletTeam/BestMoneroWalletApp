// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_CONTACTROW_H
#define BESTWALLET_CONTACTROW_H

#include <QString>

struct ContactRow
{
    QString address;
    QString label;

    ContactRow(const QString address, const QString& label)
        : address(address)
        , label(label) {}
};

#endif //BESTWALLET_CONTACTROW_H
