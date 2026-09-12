---
title: Primary address
nav_title: Primary address
category: faq
---

### What it is

Every Monero wallet has exactly one primary address. On mainnet it begins with a `4`.

It is a perfectly valid address and funds sent to it arrive normally — but you should prefer a subaddress unless something forces your hand.

Situations that genuinely call for the primary address:

- Solo mining
- [Creating a view-only wallet](create-view-only-wallet)
- Dealing with a service that has not implemented subaddress support

If a service rejects addresses starting with `8`, it is worth asking them to add subaddress support. The change is small on their side.

### Why the Receive tab hides it

Keeping it out of sight discourages address re-use, which is the main thing subaddresses exist to prevent.

### Showing it anyway

Right-click the table header on the **Receive** tab and enable **Show change address**. If the address has already received funds at some point, you may also need **Show used addresses**.

### Finding it directly

It is listed under **Wallet → Keys**.
