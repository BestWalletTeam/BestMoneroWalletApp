---
title: Accounts
nav_title: Accounts
category: faq
---

A single wallet can hold any number of accounts. Each one keeps its own balance, its own history and its own set of addresses, while all of them derive from the same private keys — so your seed alone is enough to bring every account back.

**Reasons to split things up**

- Bookkeeping: keeping personal, business and donation activity apart.
- Separating identities you don't want linked. (**Caution:** accounts are no defence against the [Janus attack](https://web.getmonero.org/2019/10/18/subaddress-janus.html).)

**Why an account instead of another wallet file**

A second wallet file means a second [synchronization](synchronization) pass over the chain. An extra account costs almost nothing by comparison — scanning speed is essentially unaffected by how many accounts you have.

[Switch subaddress account](switch-subaddress-account) covers creating accounts and moving between them.

**Limitation**

A transaction cannot draw on outputs from more than one account. Spending has to happen from within a single account.
