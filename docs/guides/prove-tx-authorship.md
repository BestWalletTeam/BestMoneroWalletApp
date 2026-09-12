---
title: How to prove authorship of a transaction
nav_title: Prove authorship of transaction 
category: howto
---

A SpendProof demonstrates that you are the one who created a transaction. It says nothing about who was paid or how much — if that is what you need, use [prove payment](prove-payment) instead.

**Creating the proof**

1. Open the **History** tab.
2. Right-click the transaction and choose **Create tx proof**.
3. Pick **Prove authorship of a transaction (SpendProof)**.
4. Optionally type something into **Message**; whatever you write is bound into the proof.

**Getting it out**

- **Get Formatted Proof** produces a self-contained block that looks like a PGP message, wrapped in `-----BEGIN SPENDPROOF-----` markers. It carries the transaction ID, the message and the signature together. Support for this layout is not universal across Monero wallets.
- **Get Signature** gives you the bare signature, which you then pass along with the other details yourself.
