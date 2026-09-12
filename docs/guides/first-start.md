---
title: First start
nav_title: First start
category: getting-started
weight: 0
---

The first time BestWallet runs, a short wizard asks how it should reach the Monero network. This page walks through those questions.

Stuck on something, or want to propose a change? [Get in touch](report-an-issue). The **Troubleshooting** section further down the sidebar covers the problems that come up most often.

---

## How do you want to connect to the Monero network?

Your wallet cannot find your transactions on its own — it needs a node, a machine holding a copy of the blockchain, to feed it blocks. [Nodes](nodes) explains the trade-offs; it is worth reading before you choose, because leaning on someone else's node carries privacy implications that may or may not matter to you.

### Auto connect

BestWallet ships with a list of well-performing nodes run by the developers and by trusted people in the Monero community, and picks from it at random.

**This is the right choice for most people.**

### A remote node of your own choosing

Pick **Select node manually** and enter its address and port, for instance `node.monerodevs.org:18089`.

### A node on your own machine

A [local node](local-node) listening on the default port is detected automatically.

BestWallet will not start or supervise a node for you. If you want to run one, [this guide](https://moneroguides.org/tutorials/01x02-setting-up-your-own-node/) covers the setup.

## How should BestWallet route its network traffic?

The default sends everything over Tor except the initial blockchain scan. Scanning shifts a lot of data and is very slow over Tor, while a remote node learns little from serving you blocks ([Nodes](nodes) goes into why) — so this is a deliberate trade of a small amount of cover for a usable sync time.

Depart from the default if either applies:

- Tor is unreachable from your machine
- Your threat model requires Tor for everything, without exception

Further reading: [Tor support](tor-support) and [Network traffic](network-traffic).

## Do you want to obtain third-party data?

The last page offers a connection to a service that supplies exchange rates, a maintained list of remote nodes, current blockheight and the Home feeds.

It is a one-way fetch. Nothing about your wallet, your balance or your transactions is sent to it, and it carries no telemetry, crash reporting or client identifier.

Declining is a legitimate choice and costs you only the features that depend on that data. You can change your mind later under **Settings → Network**.
