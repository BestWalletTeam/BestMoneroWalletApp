---
title: Paths
nav_title: Paths
category: faq
---

Two directories matter: the one holding your wallets, and the one holding the program's own data.

| | Linux | macOS | Windows |
|---|---|---|---|
| [Wallet files](wallet-files) | `~/Monero/wallets/` | `~/Monero/wallets/` | `C:\Users\<USER>\Documents\Monero\wallets\` |
| Program data | `~/.config/bestwallet/` | `~/Library/Application Support/Best Wallet/` | `C:\Users\<USER>\AppData\Local\Best Wallet\` |

Where wallets are stored is configurable — see [Wallet files](wallet-files).

**Inside the program data folder**

| File | What it holds |
|------|---------------|
| `settings.json` | Every setting you have changed |
| `tor/` | The bundled Tor binaries, unpacked here on first run, plus Tor's own state under `tor/data` |
| `pricehistory.json` | Cached XMR price history used by the portfolio chart |
| `libwallet.log*` | Logs from the underlying Monero wallet code. Not written unless logging is turned on. |

**Where these rules don't apply**

Under [portable mode](portable-mode) and on Tails, all of the above — wallet files included — goes into a single `bestwallet_data` folder next to the program file instead.

An installation that still has a `feather_data` folder from an earlier build keeps using it, so nothing has to be moved by hand.

On Linux, settings from an earlier `~/.config/feather/` directory are copied across on first launch. The old directory is left untouched, so an older build still finds what it expects.
