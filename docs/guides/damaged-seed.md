---
title: Recovering a damaged Polyseed
nav_title: Recovering a damaged Polyseed
category: advanced
---

If your written seed has become partly unreadable, the Seed Recovery tool can search for the missing pieces.

### What it can work with

- Words you cannot quite read — poor handwriting, smudging, water damage
- Words that are only partly there — cut off, abbreviated
- One or two words gone entirely, provided you know **where** in the phrase they sat

What it cannot do is reconstruct the order. If you no longer know which word went where, the tool has nothing to work from.

### First, confirm what kind of seed you have

The tool handles Polyseeds only, which are 16 words. A different length points elsewhere:

| Words | Likely scheme |
|-------|---------------|
| 12 | An Electrum (Bitcoin) seed |
| 14 | A legacy seed from an older version of this wallet's predecessor |
| 24 or 25 | A standard Monero seed |

### Try reading it against the wordlist

Before running any search, compare the doubtful words against the BIP-39 English wordlist — often that alone resolves ambiguous handwriting:

https://raw.githubusercontent.com/bitcoin/bips/master/bip-0039/english.txt

### Running the tool

Choose **Restore wallet from seed** on the main menu, then press **Ctrl + K** on the following page.

Put each word you are sure of into its own box. Partial knowledge is expressed as a regular expression:

| You know | Enter |
|----------|-------|
| It starts with "his" | `his` |
| It ends with "ory" | `ory$` |
| It starts with "hi" and ends with "y" | `^hi.*y$` |
| It contains "si" somewhere | `.*si.*` |
| It is either "history" or "victory" | `history\|victory` |
| Nothing at all | leave the box empty |

Press **Check** to begin. Every missing piece multiplies the search space, so the time required climbs steeply — with enough unknowns the search becomes impractical. Let the progress bar finish, or press **Cancel** if it is clearly not going to complete.

**If the tool says a word isn't in the wordlist:** check the spelling first, then check that your regular expression is valid. Several words failing at once usually means the phrase is not a Polyseed at all.
