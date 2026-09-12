---
title: How to sign / verify a message
nav_title: Sign / verify a message
category: howto
---

Both halves live in the same place: **Tools → Sign/verify message**.

### Signing

**Message** takes whatever you want to sign — multiple lines and special characters are fine. Leaving it empty is legitimate too, if all you are doing is proving that an address is yours.

**Address** decides which key signs. Your primary address is filled in already, but any address or subaddress belonging to the wallet works.

Press **Sign**, then **Copy to clipboard** to take all the fields at once.

Keep in mind that a signature on its own proves nothing. Verification needs the message, the address *and* the signature together, so send all three.

### Verifying

- **Message** — exactly the text that was signed, with no address or signature mixed in. Case matters.
- **Address** — the address the message was signed against.
- **Signature** — the signature itself; these begin with `SigV2`.

Press **Verify**. The result appears in a message box.
