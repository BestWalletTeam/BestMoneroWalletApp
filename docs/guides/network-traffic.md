---
title: Network Traffic
nav_title: Network Traffic
category: faq
---

BestWallet reaches out over the network in a small number of well-defined ways. Everything it does is listed below; there is nothing else running in the background.

| Destination | What it is used for | Operated by | Networking stack |
|-------------|---------------------|-------------|------------------|
| [Node](nodes) | Scanning the chain and assembling transactions | Whoever runs the node you pick | epee |
| Data service | Exchange rates, curated node list, current blockheight, Home feeds | A third party, not this project | Qt |
| api.coingecko.com | Price history for the portfolio chart | CoinGecko | Qt |

None of these connections carry telemetry, crash reports or any identifier tied
to your wallet.

Two of them are worth understanding before you rely on them. The data service is
a persistent websocket opened at launch and kept alive, and it is **not run by
this project** — its operator can see when the application starts and from which
address, and it supplies rates and node lists that the application treats as
trustworthy. The CoinGecko request is made only while the portfolio chart is on
screen. Both honour your proxy settings and both are silent in offline mode.

Node traffic and the data service are governed separately by your proxy configuration — see [Tor support](tor-support) and [i2p support](i2p-support) for how each is routed.
