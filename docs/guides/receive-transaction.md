---
title: How to receive a transaction
nav_title: Receive a transaction
category: getting-started
weight: 100
---

Open the **Receive** tab. If the table is empty, press **Create New Address**.

Any address in the list works — everything sent to any of them lands in the same account. Which one you hand out matters for privacy, not for delivery.

### Label first, copy second

Give an address a label before you pass it on, naming the person or service that will see it. For an address you are publishing somewhere, record where, or under which pseudonym.

Labelling is what stops you handing the same address to two different parties later. Reuse is the one thing worth avoiding here: give each counterparty its own address, and don't spread any single address more widely than it needs to go.

### Copying it

Right-click the address and choose **Copy Address**, or press `Ctrl + C`.

**Never transcribe an address by hand.** A single wrong character sends money somewhere unrecoverable. For the same reason, addresses are shown abbreviated in the table by default; if you genuinely need the whole string on screen, right-click the header and enable **Show full addresses**.

Moving an address to another machine safely:

- Write it to a `.txt` file and carry it on a USB drive
- Scan the QR code with the other device
- Mail it to yourself

For a large payment, it is worth asking the sender to send one piconero (0.000000000001 XMR) first. If that arrives, the address is right.

### Addresses disappearing from the list

An address is hidden automatically once it receives something — again, to steer you away from reuse. **Show used addresses** brings them back.

You can also hide one yourself: right-click **→ Hide Address**, reversed with **Show hidden addresses**.

### Q: A service says my address is invalid

Check it independently at [xmr.llcoins.net/addresstests.html](https://xmr.llcoins.net/addresstests.html): paste the address into Box 8, press **Check Address**, and read the result beside Box 14.

- **Valid** — the problem is on the service's end; a common cause is a service that has never implemented subaddress support. Tell them.
- **Invalid** — the copy went wrong somewhere. A Monero address is 95 characters. Copy it again, and if it still fails, [report](report-an-issue) it.

### Q: A payment is late

See [Help: missing transaction](missing-tx).
