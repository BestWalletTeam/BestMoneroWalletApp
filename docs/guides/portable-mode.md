---
title: How to start BestWallet in portable mode
nav_title: Enable portable mode
category: howto
---

In portable mode the wallet keeps everything beside its own executable instead of in your user profile. Nothing is written to the usual system locations.

**Turning it on**

Place an empty file next to the BestWallet program file, named any one of:

- `.portable`
- `.portable.txt`
- `portable.txt`

The check happens at startup, so the file has to exist before you launch the wallet.

**Where your data goes**

Once portable mode is active, [wallet files](wallet-files), settings and supporting data are written to a `bestwallet_data` folder alongside the executable.

If a `feather_data` folder from an earlier build is already sitting there, that one is used instead — an existing portable install carries on working untouched.

**When this is useful**

- Running the wallet from a USB stick or external disk
- Keeping everything inside an encrypted volume
- Leaving no trace in the system configuration directories

On Tails, portable mode is already the default and needs no marker file.
