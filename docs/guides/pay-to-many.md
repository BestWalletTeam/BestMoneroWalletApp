---
title: How to pay to many
nav_title: Pay to many
category: howto
---

A single transaction can pay several recipients at once. Instead of one address, type a list into the **Pay to** field.

Give each recipient its own line, written as `address, amount`:

```
85BhQjVBJzUC1D2HPfxXpMCTnadBmTjVfXRwWnWXUnovK662U5SKj9GNgPovxYSJQZLFVGZ4G3trAUar1UAMhk2bDMMDfMP, 1
8Ai1jFZTaVFGNDkXs9HfwNFBasNoxZbMEMgA39adDvop6acanDF9mtiDfqZ7o4eNb6dN4nZV37c1J8qF9rZfraAP8qYzF22, 0.5
```

The example above pays 1 XMR to the first recipient and 0.5 XMR to the second.

While you type, the **Amount** field keeps a running total of everything the transaction will send.

One transaction can carry at most 15 destinations. That ceiling comes from the Monero protocol itself, not from the wallet.
