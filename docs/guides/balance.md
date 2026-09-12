---
title: Balance
nav_title: Balance
category: faq
---

The statusbar shows the balance of whichever [account](accounts) is selected. Click it for a fuller breakdown, and adjust how it is presented under **Settings → Appearance → Balance display**.

Monero splits a balance into parts that don't always behave the way people expect. Here is what each one means.

#### Spendable

Money you can send right now.

#### Unconfirmed

Two things end up here: payments from transactions that haven't been mined yet, and outputs that are mined but still locked.

Every output is locked for 10 blocks after the transaction creating it lands — that is a Monero consensus rule, not a wallet policy. Once those confirmations accumulate the output moves into the spendable balance on its own.

Locked outputs appear in the **Coins** tab on a green background. Hover a row to see how many confirmations it still needs.

#### Total

Spendable plus unconfirmed.

### Why did part of my balance go unconfirmed after I sent something?

Because of the change. Paying someone rarely consumes an output exactly, so the remainder comes back to you as a new output — and new outputs are locked for 10 blocks like any other.

Holding more, smaller outputs reduces how much gets tied up each time. You can arrange that deliberately by [sweeping](sweep-output) one large output back to yourself and splitting it into several.

### What does freezing do to the numbers?

A frozen output is excluded from every balance, so freezing one reduces your total by its amount until you thaw it again.
