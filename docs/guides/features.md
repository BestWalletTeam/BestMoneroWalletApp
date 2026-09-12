---
title: Feature comparison
nav_title: Feature comparison
category: faq
---

How BestWallet lines up against the two reference Monero desktop wallets — the official command-line wallet (CLI) and the official graphical wallet (GUI).

| Feature | BestWallet | CLI | GUI |
|---|---|---|---|
| **► The Basics** |
| [Subaddress accounts](switch-subaddress-account) | ✔ | ✔ | ✔ |
| [Contacts](add-contact) | ✔ | ✔ | ✔ |
| Dark mode | ✔ | ✔ | ✔ |
| [View-only wallet](create-view-only-wallet) | ✔ | ✔ | ✔ |
| Language localisation | ✖* | ✔ | ✔ |
| Wallet implementation | `wallet2` <sup>[0]</sup> | `wallet2` | `wallet2` |
| **► Seed & Keys** |
| Export secret keys | ✔ | ✔ | ✔ |
| [Seed scheme](seed-scheme) | Polyseed <sup>[1]</sup> | 25 | 25 |
| Supported seed languages | 10 <sup>[2]</sup> | 12 | 12 |
| **► Privacy & Security** |
| [Built-in Tor](tor-support) | ✔ | ✖ | ✖ |
| Socks5 proxy support | ✔<sup>[3]</sup> | ✔<sup>[3]</sup> | ✔<sup>[3]</sup> |
| Address re-use mitigation | ✔ | ✔ | ✖ |
| [Encrypted wallet files](wallet-files) | ✔ | ✔ | ✔ |
| Reproducible builds | ✔ | ✔ | ✔<sup>[4]</sup> |
| Bootstrappable builds <sup>[5]</sup> | ✔ | ✖ | ✖ |
| Hide balance | ✔ | ✖ | ✔ |
| Output blackballing | ✖ | ✔ | ✔ |
| Lock on inactivity | ✔ | ✔ | ✔ |
| Lock on minimize | ✔ | ✖ | ✖ |
| **► Proofs / Messages** |
| [Sign/verify message](sign-verify-message) | ✔ | ✔ | ✔ |
| [Verify transaction proof](verify-tx-proof) | ✔ | ✔ | ✔ |
| [Formatted transaction proofs](formatted-tx-proofs) | ✔ | ✖ | ✖ |
| [Create SpendProof](prove-tx-authorship) | ✔ | ✔ | ✔ |
| [Create OutProof](prove-payment) | ✔ | ✔ | ✔ |
| Create InProof | ✔ | ✔ | ✖ |
| Create ReserveProof | ✖* | ✔ | ✖ |
| **► Coin Control** |
| Freeze/Thaw | ✔ | ✔ | ✖ |
| [Sweep single](sweep-output) | ✔ | ✔ | ✖ |
| Sweep multi (selected) | ✔ | ✖ | ✖ |
| Sweep all | ✔ | ✔ | ✖ |
| Output splitting | ✔ | ✔ | ✖ |
| Manual input selection | ✔ | ✖ | ✖ |
| Coin labeling | ✔ | ✖ | ✖ |
| **► Transactions** |
| [Multi-destination transactions](pay-to-many) | ✔ | ✔ | ✔ |
| [Transaction pusher](push-tx) | ✔ | ✖ | ✖ |
| [Manual transaction import](missing-tx) | ✔ | ✔ | ✔ |
| [Transaction rebroadcasting](failed-tx) | ✔ | ✖ | ✖ |
| Multibroadcasting | ✔ | ✖ | ✖ |
| [Offline transaction signing](offline-tx-signing) | ✔ | ✔ | ✔ |
| ... using animated QR codes | ✔ | ✖ | ✖ |
| Transaction unlock time | ✖† | ✔ | ✖ |
| Automatic network fee adjustment | ✔ | ✔ | ✔ |
| [Manual fee-tier selection](transaction-fee) | ✔ | ✔ | ✔ |
| Subtract fee from amount | ✔ | ✔ | ✖ |
| Multisig | ✖* | ✔ | ✖ |
| **► Mining** |
| Solo mining | ✖† | ✔ | ✔ |
| Pool mining | ✖† | ✖ | ✖ |
| P2Pool | ✖† | ✖ | ✔ |
| **► Hardware wallets** |
| Ledger Nano S/S+/X | ✔ | ✔ | ✔ |
| Trezor Model T/Safe 3 | ✔ | ✔ | ✔ |
| **► Fiat** |
| Crypto/fiat calculator | ✔ | ✖ | ✖ |
| Fiat balance display | ✔ | ✖ | ✔ |
| Portfolio price chart | ✔ | ✖ | ✖ |
| Historical fiat prices per transaction | ✖ | ✖ | ✖ |
| **► Misc** |
| Built-in updater | ✔ | ✖ | ✔ |
| Wallet autosave | ✔ | ✖ | ✔ |
| [Webcam QR code scanner](webcam-qr-scanner) | ✔ | ✖ | ✔<sup>[6]</sup> |
| [Export history as CSV](export-tx-history) | ✔ | ✔ | ✔ |
| Auto-open passwordless wallets | ✔ | ✖ | ✖ |
| Copy-pasteable support template | ✔ | ✖ | ✖ |
| Open multiple wallets in a single instance | ✔ | ✖ | ✖ |
| [Extra entropy from dice rolls](entropy-from-dice) | ✔ | ✖ | ✖ |
| Built-in documentation browser | ✔ | ✖ | ✖ |
| Transaction pool viewer | ✔ | ✖ | ✖ |
| [Damaged/partial seed recovery tools](damaged-seed) | ✔ | ✖ | ✖ |
| Mass address export | ✖* | ✖ | ✖ |
| Atomic swaps | ✖† | ✖ | ✖ |
| Adjustable subaddress lookahead | ✔ | ✔ | ✔<sup>[7]</sup> |
| Local node manager | ✖ | ✖ | ✔ |
| Merchant mode | ✖† | ✖ | ✔ |

### Reading the table

**✖\*** — not implemented yet  
**✖†** — no plans to implement

### Notes

**[0]** `wallet2` with modifications; see the `monero` submodule.  
**[1]** New wallets get a Polyseed. Restoring also accepts 14 and 25 word seeds — see [Seed scheme](seed-scheme).  
**[2]** Polyseed draws on the BIP-39 wordlists; Monero maintains [its own](https://github.com/monero-project/monero/tree/master/src/mnemonics). Only English is currently offered for Polyseed.  
**[3]** Proxy authentication is not supported.  
**[4]** Not on macOS.  
**[5]** See `contrib/guix` in the source tree.  
**[6]** Not in the Linux release.  
**[7]** Hardware wallets only.

Spotted something wrong or missing? [Tell us](report-an-issue).
