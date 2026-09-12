---
title: What is a node?
nav_title: Nodes
category: faq
---

A node is a machine running Monero software and holding a copy of the blockchain. Your wallet needs one to see the chain at all.

They come in two varieties:

- **Local node** — running on your own computer or inside your own network. Normally one you operate yourself.
- **Remote node** — reachable over the public internet, usually operated by somebody else. Also called a public node.

Out of the box, BestWallet picks at random from a curated list of fast remote nodes maintained by trusted people in the Monero community. The very first launch lets you decide how to connect **before** anything is contacted; afterwards the choice lives under **Settings → Network → Nodes**.

Running your own public node is well covered in [Seth's guide](https://sethforprivacy.com/guides/run-a-monero-node/).

### Q: Are remote nodes safe?

It depends what you are defending against.

**What a remote node cannot do**

- Learn your addresses
- Learn your balance
- Spend or steal your funds
- Notice your incoming transactions
- See who you are paying, or how much

**What a hostile one can attempt**

- Run an attack that may expose the real input of a transaction. It does not reveal the amount or the recipient, it rarely succeeds, and BestWallet warns you when it spots the attempt.
- Feed you a distorted fee estimate, making your transactions stand out from everyone else's.
- Tie your outgoing transactions to your IP address — but only if that traffic reaches it over clearnet. Transactions are broadcast over Tor by default; see [Tor support](tor-support).

The short version: a remote node is a metadata risk, not a custody risk. Your keys never leave your machine.

---

### For developers

Communication with the node runs over the JSON and binary [Daemon RPC](https://www.getmonero.org/resources/developer-guides/daemon-rpc.html) interface. Normal operation touches these endpoints:

| Endpoint | Stage | Notes |
|----------|-------|-------|
| `get_info` | General | Status of the node and the network. Cached for 30 seconds. |
| `/get_blocks.bin` | Sync | Block data for scanning |
| `/get_hashes.bin` | Sync | Block hashes, stored in the wallet cache as `m_blockchain` |
| `/get_transaction_pool_hashes.bin` | Sync | Pool transaction hashes; polled every 10 seconds once the main scan is done |
| `/get_transactions` | Sync | Pool transactions not yet examined |
| `/get_output_distribution` | Tx construction | Distribution used for decoy selection. The response is hashed and checked against a hardcoded value to guard against a poisoned distribution. Requested and cached the moment synchronization completes, which cuts the bandwidth a transaction needs by roughly an order of magnitude. |
| `/get_outs.bin` | Tx construction | Public keys for the chosen ring member indices. The wallet cannot validate these beyond the true input — and warns you if the true input is missing from the response. |
| `get_fee_estimate` | Tx construction | Base fee. A dishonest answer here is what creates the fungibility defect described above. |
| `hard_fork_info` | Tx construction | Current hard fork state |
| `/send_raw_transaction` | Broadcast | Hands the finished transaction to the node for relay |
