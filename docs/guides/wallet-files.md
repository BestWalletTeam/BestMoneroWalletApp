---
title: Wallet files
nav_title: Wallet files
category: faq
---

## Where they are

**Wallet → Information** reports the location of whichever wallet is currently open.

The default directory is set under **Settings → Paths → Wallet directory**. Changing it affects new wallets only; existing files stay where they are until you move them yourself.

Each wallet is two files — a keys file and a cache file — plus, on non-mainnet, a third.

## The files

### Keys file

Carries the wallet keys and settings. It takes the `.keys` extension, holds JSON once decrypted, and is encrypted with your wallet password.

`wallet2::get_keys_file_data` is the authoritative account of what goes in it.

**Warning:** deleting this file destroys the wallet unless you hold the mnemonic seed or private keys elsewhere. There is no other way back.

### Cache file

The cache — the file with no extension, also password-encrypted — holds everything the chain cannot give back: transaction data, contacts, address labels, block hashes, and the 14 word seed where one applies.

[Wallet cache viewer](wallet-cache-viewer) breaks down its contents in full.

When a cache cannot be opened, it is set aside as `<walletname>.old_cache` and a fresh one is built in its place.

**Warning:** transaction keys and your Polyseed exist only here. Neither can be reconstructed if this file is lost.

### Address file

Wallets on stagenet and testnet get one extra file, `<walletname>.address.txt`, holding the primary address in the clear. It exists so wallets belonging to different networks can be told apart.

## How the encryption works

Both encrypted files use the [ChaCha20 stream cipher](https://web.archive.org/web/20240606185144/https://www.cryptopp.com/wiki/ChaCha20). The key is derived from your password with 𝒄𝒓𝒚𝒑𝒕𝒐𝒏𝒊𝒈𝒉𝒕 — the memory-hard function Monero used for proof-of-work before ʀᴀɴᴅᴏᴍx.

From `src/crypto/slow_hash.c` in the Monero submodule:

>The 𝒄𝒓𝒚𝒑𝒕𝒐𝒏𝒊𝒈𝒉𝒕 hash operates by first using Keccak 1600,
>the 1600 bit variant of the Keccak hash used in SHA-3, to create a 200 byte
>buffer of pseudorandom data by hashing the supplied data.  It then uses this
>random data to fill a large 2MB buffer with pseudorandom data by iteratively
>encrypting it using 10 rounds of AES per entry.  After this initialization,
>it executes 524,288 rounds of mixing through the random 2MB buffer using
>AES (typically provided in hardware on modern CPUs) and a 64 bit multiply.
>Finally, it re-mixes this large buffer back into
>the 200 byte "text" buffer, and then hashes this buffer using one of four
>pseudorandomly selected hash functions (Blake, Groestl, JH, or Skein)
>to populate the output.

A single core on a modern CPU gets through the derivation in roughly 30–40 ms with the reference implementation. Purpose-built hardware and GPU implementations are orders of magnitude quicker — which is the reason a weak wallet password buys you very little.

## Using these files with other wallets

Wallet files are broadly portable across Monero wallets: create one in the CLI or the official GUI and open it in BestWallet, or the reverse. The caveat is version drift — if the underlying wallet implementations are far enough apart, compatibility breaks. Bring both clients up to date before moving a wallet between them.
