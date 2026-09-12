---
title: Wallet cache viewer
nav_title: Wallet cache viewer
category: advanced
---

The cache viewer exposes the raw `wallet2` data structures behind your wallet. It exists for developers and for anyone curious about how Monero wallets actually keep their state.

**Caution:** what this dialog shows includes sensitive material. Don't screen-share it.

Open it from **Wallet → Advanced → Wallet cache debug**.

### What each structure holds

| Structure | Contents |
|-----------|----------|
| `m_blockchain` | Block hashes |
| `m_transfers` | Outputs received by this wallet — the source of the Coins tab |
| `m_key_images` | Key image → `m_transfers` index. Sorted by index for readability. |
| `m_pub_keys` | Public key → `m_transfers` index. Sorted by index for readability. |
| `m_address_book` | Your contacts |
| `m_payments` | Confirmed incoming transactions |
| `m_unconfirmed_payments` | Incoming transactions not yet mined |
| `m_confirmed_txs` | Confirmed outgoing transactions |
| `m_unconfirmed_txs` | Outgoing transactions not yet mined |
| `m_tx_keys` | txid → transaction key |
| `m_additional_tx_keys` | Extra transaction keys per outgoing transaction |
| `m_tx_notes` | Transaction descriptions |
| `m_scanned_pool_txs` | Mempool transactions already examined |
| `m_subaddresses` | Address key → \<major index\>,\<minor index\> |
| `m_subaddress_labels` | \<major index\>,\<minor index\> → label |
| `m_account_tags` | Account index → label |
| `m_attributes` | Free-form wallet attributes, string → string |

Some structures are deliberately not rendered: `m_account_public_address`, `m_ring_history_saved`, `m_last_block_reward`, `m_tx_device`, `m_device_last_key_image_sync`, `m_cold_key_images` and `m_rpc_client_secret_key`.

### Attributes written by this wallet

These live inside `m_attributes` under a `bestwallet.` prefix.

Wallets created before the project was renamed stored the same values under a `feather.` prefix. Those are still read when the current key is absent, so no wallet loses its seed or its address lists; whenever a value is next written it moves to the current prefix.

| Attribute | Purpose |
|-----------|---------|
| `bestwallet.seed` | A stored copy of the wallet's [Polyseed](seed-scheme) |
| `bestwallet.seedoffset` | The seed offset passphrase |
| `bestwallet.hiddenaddresses` | Comma-separated list of subaddresses hidden from view |
| `bestwallet.pinnedaddresses` | Comma-separated list of pinned subaddresses |
| `bestwallet.subaddress_account` | Index of the account last selected |
| `tx:<txid>` | Hexstring of an outgoing transaction |
