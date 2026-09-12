---
title: Seed scheme
nav_title: Seed scheme
category: faq
---

New wallets get a 16 word [Polyseed](https://github.com/tevador/polyseed). Builds before 2.0.0 issued 14 word seeds from the now-deprecated [monero-seed](https://github.com/tevador/monero-seed) library.

For restoring, all three formats are accepted: Polyseed, 14 word and 25 word.

**What Polyseed improves on**

- The creation date travels inside the seed, so there is no separate [restore height](restore-height) to remember.
- It draws on the BIP-39 wordlists, whose words are shorter and more familiar than Monero's own.
- Sixteen short words are faster to write down and easier to commit to memory.

### Will my 14 word seed keep working?

Yes, indefinitely. There is no need to move your funds to a new wallet.

### Can a 14 word seed become a Polyseed?

No. A Polyseed can only come from creating a new wallet.

### Can I get a 25 word seed out of a Polyseed?

Yes. Open **Wallet → Seed**, enter your password and tick **Show 25 word seed**. Note the restore height at the same time — the 25 word form carries no creation date, so a restore without the height will scan much further back than it needs to.

### Can I go the other way, from 25 words to a Polyseed?

No. Polyseed runs its seed through a [key derivation function](https://en.wikipedia.org/wiki/Key_derivation_function) to arrive at the spend key, and that step cannot be undone.
