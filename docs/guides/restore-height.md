---
title: What is the wallet restore height?
nav_title: Restore height
category: faq
---

The restore height is the block the wallet begins [scanning](synchronization) from. Starting at the right height instead of at block zero is what keeps a restore down to minutes rather than hours.

**Usually you can ignore it**

The [seed scheme](seed-scheme) BestWallet uses stores the wallet's creation date inside the seed itself, and the restore height is worked out from that.

It only comes up when the seed carries no date:

- Restoring from a 25 word seed
- Restoring from a hardware device

**If you don't know the height**

Enter a creation date instead — the wallet converts it into a height for you. Unsure of the date? Give the earliest one that could possibly be right. If you know it was somewhere in 2019, use 2019-01-01.

Erring early costs you a longer scan. Erring late costs you data: a height set past your first transactions means those transactions are never seen, leaving you with a truncated history and an [incorrect balance](incorrect-balance).

**Looking it up later**

The current restore height of any wallet is shown in **Help → Debug info**.
