---
title: "Help: transaction did not arrive"
nav_title: Transaction did not arrive
category: troubleshooting
---

This is for the case where **you sent** a payment and the recipient says nothing turned up. If you are the one waiting on money, go to [missing transaction](missing-tx) instead.

Work down the list — each step rules out one cause.

### 1. Are you on a current version?

Monero occasionally ships consensus changes that older wallet builds cannot produce valid transactions under. A wallet that has fallen far enough behind will construct transactions the network simply rejects.

Check what you are running in **Help → About**, and compare it against the latest release in the [project repository](https://github.com/BestWalletTeam/BestWalletApp). If you are behind, update and send again.

### 2. Does the network have the transaction?

Right-click the outgoing transaction and choose **Copy → Transaction ID**, then search for it on a block explorer such as [xmrchain.net](https://xmrchain.net).

Nothing found means it never propagated — see [failed transaction](failed-tx).

### 3. Did it go to the right address?

Right-click the transaction and choose **Show details**. Compare the address under **Destinations** with the one the recipient gave you, character for character.

If they differ, the money is gone. Monero transactions cannot be reversed and funds sent to a wrong address cannot be recovered.

### 4. Prove it from your side

If the transaction is in a block and the address is correct, the problem lies with the recipient — most often a wallet of theirs that hasn't finished scanning.

Send them a [payment proof](prove-payment). It demonstrates, verifiably, that the payment was made to that address.
