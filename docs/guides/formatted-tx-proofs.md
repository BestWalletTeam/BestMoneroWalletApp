---
title: Formatted transaction proofs
nav_title: Formatted transaction proofs
category: faq
---

A transaction proof normally consists of several separate values that all have to travel together. The formatted variant packs them into a single PGP-looking block, so there is one thing to copy and one thing to paste.

Example:

```
-----BEGIN OUTPROOF-----
Network: Monero Mainnet
Txid: 46d9f3eaf8d25b6a5d0847ad0beaece8b153d1b8c25ce317934ec17223025806
Address: 463o7kTG4n2AEHNhhV91BK7kZsKd4vRh78xt6LM7Px7o3cp5eZgyuyLHVAWeEkfLcoC78WMZuJ3xZ1SiM4RhbciCB8b8pGS

A small donation.
-----BEGIN OUTPROOF SIGNATURE-----

OutProofV2SWYVZHfqBwYGNYxDRZryBdXomxA9hS3VLWiRiQZ9YNutAGoq1QxB7j
AYwRrhoxJBQ8jj7XCNNfvSwgQaVFjWY5EQHJRW3LgPtV3UmefmQ2wFJPh2iTaCH8
zETWpxTrf2RiVY
-----END OUTPROOF SIGNATURE-----
```

[Verify transaction proof](verify-tx-proof) covers checking one of these inside the wallet.

Not every Monero wallet understands this layout. When the other party is on different software, send them the individual fields instead.

---

### Format specification

Implementers can parse the block with the following expression (newlines added here for legibility):

```
-----BEGIN (?<type>\w+)-----\n
Network: (?<coin>\w+) (?<network>\w+)\n
Txid: (?<txid>[0-9a-f]{64})\n
(Address: (?<address>\w+)\n)?
\n?
(?<message>.*?)\n
-----BEGIN \1 SIGNATURE-----\n
\n?
(?<signature>.*?)\n
-----END \1 SIGNATURE-----
```

Field rules:

| Field | Rule |
|-------|------|
| `type` | One of `SpendProof`, `OutProof`, `InProof` |
| `coin` | Always `Monero` |
| `network` | One of `Mainnet`, `Stagenet`, `Testnet` |
| `txid` | 64 hexadecimal characters |
| `address` | A valid address for the stated `network`. Required for `OutProof` and `InProof`; must be absent for `SpendProof`. |
| `message` | Optional, arbitrary length, may span lines |
| `signature` | The proof signature. Should be wrapped at 64 characters per line. |
