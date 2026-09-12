---
title: i2p support
nav_title: i2p support
category: faq
---

BestWallet can push all of its network traffic through I2P. You supply the router; the wallet only needs to be told where to find it.

### Setting up i2pd

Grab a current i2pd build from the project's release page: https://github.com/PurpleI2P/i2pd/releases

- Windows: `i2pd_x.xx.x-win64-mingw.zip`
- macOS: `i2pd_x.xx.x-osx.tar.gz`

Unpack the archive and start the `i2pd` executable. Leave it running while you use the wallet.

### Pointing BestWallet at it

Open **Settings → Network → Proxy**, choose **i2p**, and confirm with **Apply**.

With **Let BestWallet manage this list** ticked on the **Node** tab, the wallet will pick an i2p node on its own from that point on.

The proxy setting is not limited to node traffic — every other connection the wallet makes goes through I2P as well.
