---
title: Transaction pusher
nav_title: Transaction pusher
category: advanced
---

The transaction pusher takes a raw, already-signed transaction and hands it to a node for you. It is the tool you reach for after signing on an offline machine, or when a transaction needs another push onto the network.

Everything it sends travels over Tor. The one exception is a local node, which is contacted directly.

**Steps**

1. Open **Tools → Broadcast transaction → From text**.
2. Paste the transaction hexstring into the **Transaction** box.
3. Under **Node**, choose between the node you are currently connected to and a node you specify yourself.
4. Press **Broadcast**.

Give the network a few minutes afterwards. If the transaction belongs to the wallet you have open, it will appear in the **History** tab once it has been relayed.
