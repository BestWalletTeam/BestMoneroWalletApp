---
title: How to show the wallet seed
nav_title: Show wallet seed
category: howto
---

Open **Wallet → Seed**, or click the seed icon in the status bar. You will be asked for your password before the seed is revealed.

Two kinds of wallet have no seed to show: view-only wallets, and wallets whose keys live on a hardware device.

**Polyseed and the 25 word form**

Wallets created in BestWallet use Polyseed, which is 16 words long. Tick **Show 25 word seed** to render the same wallet as a legacy 25 word seed — useful when you need to restore into software that has no Polyseed support.

If you go that route, write down your restore height as well. The 25 word form does not carry a creation date, so without the height a restore will rescan far more of the chain than necessary.

[Seed scheme](seed-scheme) explains what Polyseed is and why it is used.
