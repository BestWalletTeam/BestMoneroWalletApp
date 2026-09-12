---
title: "Help: Incorrect balance"
nav_title: Incorrect balance
category: troubleshooting
---

Work through the checks below when the figure your wallet reports doesn't match what you expect.

If the problem is specifically that an incoming payment never showed up, that is a different situation — see [Help: Missing transaction](missing-tx).

### 1. Check for frozen coins

Frozen outputs are excluded from the total. Open the **Coins** tab (**View → Coins**) and look for rows with a blue background. [Thawing](freeze-thaw-output) them puts the amount back into your spendable balance.

### 2. Rescan spent outputs

The wallet can re-check which of your outputs have actually been spent: **Wallet → Advanced → Rescan spent**.

### 3. Rebuild from the seed

If neither step above helps, [restore the wallet from its seed](restore-from-seed). This discards the local cache and rebuilds your history from the chain.
