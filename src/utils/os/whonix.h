// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_WHONIX_H
#define BESTWALLET_WHONIX_H

#include <QString>

struct WhonixOS {
    static bool detect();
    static QString version();
};


#endif //BESTWALLET_WHONIX_H
