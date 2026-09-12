---
title: How to create/restore a wallet from a hardware device
nav_title: Create a wallet (hardware device)
category: getting-started
weight: 20
---

Check first that your device is among the [supported hardware devices](hardware-wallet-support).

### Before you start

Connect the device and get it ready:

- **Ledger** — open the Monero app on the device.
- **Trezor** — unlock it.

On Linux, the device also needs udev rules in place before anything can talk to it:

- **Ledger**: [add_udev_rules.sh](https://github.com/LedgerHQ/udev-rules/blob/master/add_udev_rules.sh)
- **Trezor**: [trezor.io/learn/a/udev-rules](https://trezor.io/learn/a/udev-rules)

### In the wizard

Open **File → New/Restore** if the wizard isn't up, then choose **Create wallet from hardware device** and select your device type on the following page. A device that isn't in that list is not supported yet.

Now pick which of the two you are doing:

- **Create new wallet file from device** — this is a fresh start. Use it when the device has never held a Monero wallet, or when its keys have no funds behind them.
- **Restore a wallet from device** — use it when Monero has previously moved to or from this device.

A restore asks for the [restore height](restore-height). If you don't know it, put the date you bought the device into **Wallet creation date** and a height will be derived from that.

From there the wizard continues as normal: name the wallet files and set a password.
