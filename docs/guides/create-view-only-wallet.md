---
title: How to create a view-only wallet
nav_title: Create a view-only wallet
category: howto
---

A view-only wallet holds the view key but not the spend key. It can see money arriving, but it cannot spend, and it has no way of telling which of your outputs you have already used. That last point matters: as soon as the wallet has outgoing transactions, the balance a view-only copy shows will be too high until you import the matching key images.

The Monero project's [view-only wallet guide](https://www.getmonero.org/resources/user-guides/view_only.html) covers the limitations and typical uses in more depth.

### Deriving one from a wallet you already have

Open **Wallet → View-Only** and click **Create view-only wallet**. Give it a name, click **Save**, and set a password if you want one.

### Building one from keys

Choose **Restore wallet from keys** in the wizard. Supply the primary address and the secret view key, then click **Next**. The following page asks for a restore height or the date the wallet was created. From there the wizard continues as it would for any other wallet.
