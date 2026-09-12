---
title: Tor support
nav_title: Tor support
category: faq
---

Tor works in BestWallet without any setup on your part. You do not need to install it separately.

**How the daemon is provided**

A Tor binary ships with the wallet. On startup BestWallet looks for a Tor daemon already listening on the standard port (9050); if it doesn't find one, it unpacks its own copy into the [config folder](paths) and runs it on port 19450.

**Routing modes for node traffic**

- Never over Tor
- Clearnet during the initial scan, Tor afterwards (**default**)
- Always over Tor

The choice is offered the very first time you run the wallet, before it opens any connection at all. It can be changed later under **Settings → Network → Proxy**, and you can point the wallet at a different proxy — or none — if Tor is unreachable on your machine.

**Why synchronization is the exception**

Scanning transfers a great deal of data and is painfully slow over Tor, while a remote node learns very little from serving you blocks ([Nodes](nodes) explains why). Trading that particular bit of cover for a usable sync time is a deliberate call.

**Cases where the setting doesn't apply**

Under Tails and Whonix, or when the wallet is launched through `torsocks`, the system routes everything over Tor no matter what the wallet is configured to do. In the other direction, traffic to a [local node](local-node) never goes through Tor.
