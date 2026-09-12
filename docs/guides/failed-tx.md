---
title: "Help: Failed transaction"
nav_title: Failed transaction
category: troubleshooting
---

This applies when an outgoing transaction is marked **Failed** in the **History** tab.

### First try: resend it

Right-click the failed transaction and choose **Resend transaction**, then click **Broadcast**.

Give it a few minutes, then look for it in a block explorer's pool view, such as [xmrchain.net](https://xmrchain.net/txpool). If it never turns up, switch to a different node and broadcast again.

### Why this happens

The transaction itself is fine — what failed is the node's job of passing it on to the rest of the network. Nodes that are misconfigured or badly connected drop transactions this way.

Nodes running with `--tx-proxy` are a frequent culprit. Operators of public nodes are better off leaving that option off.

### Relaying it yourself from a local node

With your own daemon you can push the transaction out directly. In the daemon console:

```
relay_tx <txid>
```

For example:

```
relay_tx 210557aa553718f8faced109768c54763702847e52f66f9ebdee7aceee90c4bc
```

When `monerod` runs as a daemon:

```
monerod relay_tx <txid>
```

Or over JSON-RPC, substituting your own transaction ID for `TXID_HERE`:

```
curl http://127.0.0.1:18081/json_rpc -d \
    '{"jsonrpc":"2.0", \
      "id":"0", \
      "method":"relay_tx", \
      "params":{"txids":["TXID_HERE"]}}' \
    -H 'Content-Type: application/json'
```
