---
title: How to sweep outputs
nav_title: Sweep outputs
category: howto
---

### What a sweep is

A sweep sends the entire value of the outputs you pick, less the fee, to one destination. Because nothing is left over, no change output is produced.

### Doing it

1. Enable the **Coins** tab under **View → Coins**.
2. Select the outputs you want to move. Hold Ctrl to pick more than one.
3. Right-click the selection and choose **Sweep Output(s)**.
4. In the dialog, put the destination in the **Address** field — or tick **Send to self (churn)** to send it to your own primary address.

**Number of outputs** lets you break the swept total into that many equal pieces. A transaction can create at most 16, which is a limit of the Monero protocol.

### When it's the right tool

- Shifting outputs between accounts or wallets without leaving change behind
- Consolidating scattered outputs, or churning specific ones

For ordinary payments use the **Send** tab instead. If you simply want to empty the wallet, the **Max** button there does that.
