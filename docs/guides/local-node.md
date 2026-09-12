---
title: How to connect to a local node
nav_title: Connect to local node
category: howto
---

**Steps**

1. Open **Settings → Network → Node**.
2. Clear the **Let BestWallet manage this list** checkbox.
3. Click **Add custom node(s)**.
4. Type the node's `IP:port` into the box.

A node running on the same machine as the wallet answers on `127.0.0.1`. Which port depends on the network monerod was started for:

| Network  | Port  |
|----------|-------|
| MAINNET  | 18081 |
| TESTNET  | 28081 |
| STAGENET | 38081 |

So a mainnet daemon on your own machine is reached at `127.0.0.1:18081`.

**What counts as local**

Addresses in these IPv4 ranges are treated as local:

- 127.0.0.0/8
- 172.16.0.0/12
- 192.168.0.0/16

Traffic to a local node is sent directly rather than through Tor. Routing it over Tor would add latency without hiding anything — the node is already on your own machine or network.
