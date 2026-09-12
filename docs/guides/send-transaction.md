---
title: How to send a transaction
nav_title: Send a transaction
category: getting-started
weight: 110
---

### Paying one address

Everything happens on the **Send** tab.

**Pay to** accepts a Monero address. It will also take an [OpenAlias](https://openalias.org/) — an email-shaped name that resolves to an address — and you can paste an image of a QR code straight into the field, whether it encodes a plain address or a full payment request.

**Description** is for you alone. It never leaves your machine, is kept in the wallet cache, and shows up beside the transaction in **History**.

**Amount** is what the recipient ends up with; the fee is added on top rather than taken out. **Max** empties the account.

Press **Send** to build the transaction. Nothing is broadcast yet.

Hardware wallet users: the device will ask you to approve the operation, and building a transaction on-device can take several minutes.

### Reviewing before it goes out

A confirmation dialog appears once the transaction exists. Check the destination and the fee here. **Advanced** opens the full picture, including which of your outputs are being spent.

**Send** broadcasts it. **Cancel** discards it, at no cost.

### Afterwards

The transaction appears in **History** with a gears icon while it waits to be mined.

If several minutes go by and your node's mempool never shows it, the status flips to **Failed**. Confirm that by searching a public mempool such as [xmrchain.net](https://xmrchain.net/txpool) — if it is absent there too, the node did not relay it, and [failed transaction](failed-tx) explains what to do.

### Other kinds of send

- Several recipients in one transaction: [Pay to many](pay-to-many)
- Spending specific outputs in full: [Sweep outputs](sweep-output)
