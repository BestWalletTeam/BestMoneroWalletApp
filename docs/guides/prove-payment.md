---
title: How to prove a payment
nav_title: Prove payment 
category: howto
---

### OutProof

An OutProof is the normal way to show that you paid a particular address.

1. Go to the **History** tab.
2. Right-click the transaction containing the payment and choose **Create tx proof**.
3. Select **Prove payment to an address**.
4. Confirm that the destination address shown is the right one, and add a message if the recipient asked for one.

Then pick how to hand it over:

- **Get Formatted Proof** bundles the transaction ID, address, message and signature into one PGP-style block. It is the easiest thing to send, but the recipient's wallet has to understand the format.
- **Get Signature** returns just the signature. Use this when the other side is on different software.

Whoever verifies the payment needs four things: the signature, the address, the message and the transaction ID. The formatted block already contains all four; if you send a bare signature, send the rest alongside it.

### Transaction secret key

Some exchanges and merchants ask for a transaction secret key instead of a proof. Find it in the **History** tab: right-click the transaction, choose **Show details**, then click **Copy Tx Secret Key**.
