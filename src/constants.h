// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_CONSTANTS_H
#define BESTWALLET_CONSTANTS_H

#include <QString>

#include "networktype.h"

namespace constants
{
    extern NetworkType::Type networkType; // TODO: not really a const

    // Set by --use-local-tor for this run only, rather than written to the
    // config where nothing would clear it.
    extern bool useLocalTorOverride;

    // coin constants
    const std::string coinName = "monero";
    const qreal cdiv = 1e12;
    const quint32 mixin = 10;
    const quint64 kdfRounds = 1;

    const QString seedLanguage = "English"; // todo: move me

    // Theme identifiers. Persisted in the config, so the settings combo box
    // and WindowManager's skin table must agree on these exact strings.
    const QString skinNativeDark = "Native (dark)";
    const QString skinNativeWhite = "Native (white)";

    // Brand accent, shared by both skins. Kept here because the stylesheets
    // and the code that recolours artwork have to agree on it.
    const QString accentColor = "#FE681B";

    // The project's own site. Links here skip the external link warning, which
    // exists to flag handing the user off to a third party. Anything built on
    // this must compare the parsed host, never the raw URL string.
    const QString websiteDomain = "bestmonerowallet.app";
    const QString websiteUrl = "https://" + websiteDomain;

    // Base name of a published release archive, as in
    // "<name>-<version>-<platform>.zip". The updater looks the hash up under
    // this name and writes the download under it.
    const QString releaseArtifactName = "bestwallet";

    // The executable inside the macOS app bundle. Must track the CMake target name.
    const QString macBundleExecutable = "BestWallet";
}

#endif //BESTWALLET_CONSTANTS_H
