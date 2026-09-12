---
title: "Help: missing transaction"
nav_title: Missing transaction
category: troubleshooting
---

For when a payment you expect to receive has not appeared. Take these in order.

### 1. Has the wallet finished scanning?

An incoming transaction is invisible until the scan reaches its block. The statusbar counts down the remaining blocks and reads **Synchronized** when it is done — wait for that before concluding anything is wrong. See [Synchronization](synchronization).

### 2. Are you on a current version?

Monero's consensus rules change from time to time, and builds that fall far enough behind stop working against the network altogether. Newer builds also carry bug and security fixes worth having.

**Help → About** shows your version; compare it against the latest release in the [project repository](https://github.com/BestWalletTeam/BestWalletApp).

### 3. Does the network have the transaction?

Look the transaction up on a public block explorer such as [xmrchain.net](https://xmrchain.net).

If it isn't there, it was never relayed properly. Tell the sender — they can [rebroadcast](failed-tx) it.

### 4. Import it by hand

If the explorer has it but your wallet does not, pull it in directly: [import transaction](import-transaction).

### 5. Check the address it went to

Should the import report that the transaction doesn't belong to your wallet, verify the destination. Open **Tools → Address checker** and paste in the address the sender used.

If that address isn't yours, one of these happened:

- You have a different wallet file open than you think
- The address was mistyped or mangled on the way to the sender ([never type an address by hand](receive-transaction))
- [Clipboard-hijacking malware](https://medium.com/chainreport/copy-paste-malware-crypto-multipliers-4a0c09c7730a) swapped it out on one of your machines

Funds sent to an address that isn't yours cannot be recovered.

### 6. Ask for proof

If nothing above resolves it, ask the sender for an OutProof or the transaction secret key.

[Verify transaction proof](verify-tx-proof) covers checking a proof in the wallet. A transaction secret key can be checked at [xmrchain.net](https://xmrchain.net) — look up the transaction ID and use **Prove sending**.

A sender who cannot produce a valid proof may not have sent anything at all.
