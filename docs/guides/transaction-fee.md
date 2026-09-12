---
title: Transaction Fee
nav_title: Transaction Fee
category: faq
---

BestWallet sets the fee for you, scaling it against how busy the mempool is. This matches the "automatic" fee behaviour of the official GUI.

#### Can I pick the fee tier myself?

Yes. Open the settings, go to the transactions tab, and turn on **manual fee-tier selection**. A tier selector then appears on the **Send** tab.

#### My transaction still hasn't confirmed

Start by checking whether the network has it at all — paste the transaction ID into a block explorer such as [xmrchain.net](https://xmrchain.net).

- **Not found?** The transaction likely never propagated. Rebroadcast it using the steps in [failed transaction](failed-tx).
- **Found, but unconfirmed?** There is nothing further to do from the wallet. It should be mined shortly, unless the network is under unusual load — a spam wave or an upgrade, for instance. The Monero community channels are the place to check for that.

#### Where does the fee go?

To whoever mines the block your transaction lands in. No part of it is taken by the BestWallet developers.
