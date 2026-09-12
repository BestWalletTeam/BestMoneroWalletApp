// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_WALLETATTRIBUTES_H
#define BESTWALLET_WALLETATTRIBUTES_H

#include <QString>

// Names of the attributes this application stores inside the wallet cache, a
// shared key/value store where clients prefix their keys to avoid collisions.
//
// Earlier builds wrote under the legacy prefix below, and one of these
// attributes holds the Polyseed backup the seed dialog displays. Reads fall back
// to the legacy prefix, and the value is rewritten under the current one the
// next time it is set.
namespace WalletAttributes
{
    // Prefix for everything this application writes from now on.
    inline constexpr char prefix[] = "bestwallet.";

    // Prefix used before the project was renamed. Read-only: never written.
    inline constexpr char legacyPrefix[] = "feather.";

    // Bare attribute names, without a prefix.
    inline constexpr char seed[]              = "seed";
    inline constexpr char seedOffset[]        = "seedoffset";
    inline constexpr char hiddenAddresses[]   = "hiddenaddresses";
    inline constexpr char pinnedAddresses[]   = "pinnedaddresses";
    inline constexpr char subaddressAccount[] = "subaddress_account";

    inline QString key(const QString &name) {
        return QString::fromLatin1(prefix) + name;
    }

    inline QString legacyKey(const QString &name) {
        return QString::fromLatin1(legacyPrefix) + name;
    }
}

#endif //BESTWALLET_WALLETATTRIBUTES_H
