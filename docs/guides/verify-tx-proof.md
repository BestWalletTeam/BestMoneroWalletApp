---
title: How to verify a transaction proof
nav_title: Verify transaction proof
category: howto
---

Three proof types can be checked in the wallet:

| Proof | What it establishes |
|-------|---------------------|
| SpendProof | The prover created the transaction |
| OutProof | A payment was made to a given address |
| InProof | The prover owns an output |

ReserveProofs cannot be verified here yet.

Start from **Tools → Verify transaction proof**.

### Formatted proofs

A formatted proof arrives as a single block bracketed by `-----BEGIN ...-----` lines, in the style of a PGP message. Everything the check needs is already inside it.

Open the **Formatted** tab, paste the block in, and press **Verify**. A valid proof is confirmed with a green checkmark and the message "Proof is valid".

### Loose proofs

If you were given the pieces separately, use the **Manual** tab. Choose which proof type you are checking, fill in the fields you were given, and press **Verify**. The result is reported in a message box.
