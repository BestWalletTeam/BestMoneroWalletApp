---
title: How to restore a wallet from seed
nav_title: Restore wallet from seed
category: getting-started
weight: 30
---

Open the wizard with **File → New/Restore** if it is not already in front of you, then choose **Restore wallet from seed** and pick the seed type you hold.

Type the words into the box separated by single spaces. Take your time here: a misspelled word or two words in the wrong order produces a different, empty wallet rather than an error.

Seed passphrases are set under **Options → Extend this with a passphrase**; the wizard prompts for the passphrase itself on the following page.

You will then be asked for a [restore height](restore-height), or the date the wallet was created if you don't know the height. The rest of the wizard runs as usual.

## What a seed does and does not bring back

Everything below is derived from the seed itself, so it comes back automatically:

- Balance
- Transaction history
- Accounts and subaddresses

The following only ever existed in your local wallet files and cannot be reconstructed from the chain:

- Contacts
- Labels on accounts and subaddresses
- Transaction descriptions
- Transaction keys
- The destination addresses of transactions you sent

If those matter to you, keep a backup of the wallet files themselves — see [Wallet files](wallet-files).
