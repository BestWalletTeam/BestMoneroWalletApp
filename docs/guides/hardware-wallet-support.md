---
title: Hardware wallets
nav_title: Hardware wallets
category: faq
---

### Are hardware wallets supported?

**Yes.** These devices work with BestWallet:

| Vendor | Models |
|--------|--------|
| Ledger | Nano S, Nano S+, Nano X, Stax, Flex |
| Trezor | Model T, Safe 3, Safe 5 |

Setup instructions are in [Create a wallet (hardware device)](create-wallet-hardware-device).

The **Trezor Safe 7** does not work yet. It moved to a new device communication protocol, and there is no timeline for that protocol landing in the Monero codebase.

Own a device that advertises Monero support but isn't listed above? [Let us know](report-an-issue).

### How the supported devices compare

Secure element, passphrase support, connectivity and firmware openness all differ by model:

| Device | Secure element | Peripherals on the SE | Passphrase | Connectivity | Open firmware |
|---|---|---|---|---|---|
| Ledger Nano S | [ST31H320](https://www.st.com/en/secure-mcus/st31h320.html) <sup>[1]</sup> | No | Yes <sup>[5]</sup> | USB | No |
| Ledger Nano S+ | [ST33K1M5](https://www.st.com/en/secure-mcus/st33k1m5c.html) <sup>[3]</sup> | Yes | Yes <sup>[5]</sup> | USB | No |
| Ledger Nano X | [ST33J2M0](https://www.st.com/en/secure-mcus/st33j2m0.html) <sup>[2]</sup> | Yes | Yes <sup>[5]</sup> | USB, Bluetooth | No |
| Ledger Stax | [ST33K1M5](https://www.st.com/en/secure-mcus/st33k1m5c.html) <sup>[6]</sup> | Yes | Yes <sup>[5]</sup> | USB, Bluetooth | No |
| Ledger Flex | [ST33K1M5](https://www.st.com/en/secure-mcus/st33k1m5c.html) <sup>[10]</sup> | ? | Yes <sup>[5]</sup> | USB, Bluetooth | No |
| Trezor Model T | None | No | Yes <sup>[7]</sup> | USB | Yes, reproducible <sup>[4]</sup> <sup>[8]</sup> |
| Trezor Safe 3 | OPTIGA™ Trust M (V3) <sup>[9]</sup> | ? | Yes <sup>[7]</sup> | USB | Yes, reproducible <sup>[4]</sup> <sup>[8]</sup> |
| Trezor Safe 5 | OPTIGA™ Trust M (V3) <sup>[9]</sup> | ? | Yes <sup>[7]</sup> | USB | Yes, reproducible <sup>[4]</sup> <sup>[8]</sup> |

A "?" means the answer is not publicly documented in a form we can cite.

The two columns worth understanding before choosing:

- **Secure element** — dedicated tamper-resistant hardware for key storage. The Trezor Model T has none and relies on a general-purpose microcontroller.
- **Open firmware** — whether you can read the code running on the device, and whether builds of it are reproducible. Ledger's firmware is closed; Trezor's is not.

### References

**[1]** https://www.ledger.com/improving-and-supporting-the-ledger-nano-s  
**[2]** https://shop.ledger.com/products/ledger-nano-x#shopify-section-tech-specs--nano-x  
**[3]** https://www.ledger.com/blog/ledger-op3n-conference-product-launches-updates-our-next-milestones  
**[4]** https://docs.trezor.io/trezor-firmware/common/reproducible-build.html  
**[5]** https://www.ledger.com/academy/passphrase-an-advanced-security-feature  
**[6]** https://shop.ledger.com/products/ledger-stax  
**[7]** https://trezor.io/learn/a/passphrases-and-hidden-wallets  
**[8]** https://github.com/trezor/trezor-firmware  
**[9]** https://trezor.io/learn/a/secure-element-in-trezor-safe-devices  
**[10]** https://shop.ledger.com/products/ledger-flex
