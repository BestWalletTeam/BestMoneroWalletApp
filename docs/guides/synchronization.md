---
title: What is wallet synchronization?
nav_title: Synchronization
category: faq
---

Synchronization — also called a wallet refresh — is the wallet working through the blockchain looking for transactions that belong to you.

**How it works**

The wallet pulls blocks from a [node](nodes) in order, picking up where it left off. The point it has reached is the *wallet height*, visible under **Help → Debug Info**. Anything that turns out to be yours is written to the [wallet cache](wallet-files); everything else is thrown away immediately.

Until the scan reaches a given block, transactions in it are invisible to you — absent from your history and not counted in your balance. The status bar reports **synchronized** once the wallet has caught up.

**What the node learns**

Nothing about your transactions. All matching happens on your own machine; the node only ever sees requests for blocks.

**Speed and Tor**

Scanning moves a lot of data, and over [Tor](tor-support) it slows to a crawl. The default configuration therefore synchronizes over clearnet, on the reasoning that a node learns very little during a scan anyway (see [Nodes](nodes) for the detail).

That default does not apply where the operating system routes everything through Tor regardless — Tails and Whonix, for example.
