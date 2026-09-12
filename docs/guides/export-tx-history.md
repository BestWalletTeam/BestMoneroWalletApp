---
title: How to export the transaction history
nav_title: Export transaction history
category: howto
---

Choose **Wallet → History → Export CSV**, pick a filename and click **Save**.

The resulting CSV has one row per transaction, with these columns:

| Field | Type | Example | Meaning |
|-------|------|---------|---------|
| blockHeight | integer | 2317852 | Block the transaction was mined into |
| timestamp | integer | 1615848678 | Unix time that block was mined, approximately |
| date | string | 2021-03-15T23:51:18Z | The same moment in ISO 8601 form |
| accountIndex | integer | 0 | The [account](accounts) that sent or received it. Transfers between your own accounts appear more than once. |
| direction | string | out | `in` or `out`. Churn counts as outgoing. |
| balanceDelta | float | -0.202417740000 | Net effect on your balance; negative when you sent |
| amount | float | 0.202400000000 | Absolute value sent or received, **fee not included**. Churn shows 0. |
| fee | float | 0.000017740000 | Network fee. On incoming rows this is what the *sender* paid. |
| txid | string | 449f065...876206 | Transaction ID (shortened here for readability) |
| description | string | Donation to Monero Core | Whatever description you attached |
| paymentID | string | | Payment ID; normally empty |
