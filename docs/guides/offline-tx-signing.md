---
title: Offline transaction signing
nav_title: Offline transaction signing
category: advanced
---

Running into problems? [Help: Airgapped signing issues](airgapped-signing-issues) collects the common ones.

## What this is

Offline — or airgapped — signing keeps your secret spend key on a machine that has never touched the internet. A hardware wallet does the same thing in a sealed package; here you assemble it yourself from two ordinary computers.

You need both of these:

- **The online device**, holding a view-only wallet. It watches the chain, builds transactions and broadcasts them, but cannot sign anything.
- **The offline device**, holding the matching wallet with the spend key. Usually a laptop or desktop with its network hardware removed, used for nothing else. It signs, and does nothing more.

Building and hardening that offline machine is beyond this guide, but a minimal Linux install on an [encrypted filesystem](https://wiki.archlinux.org/title/Dm-crypt/Encrypting_an_entire_system) is a sound starting point.

The cycle is always the same: the online wallet composes the transaction, the offline wallet signs it, and the online wallet — or any [transaction pusher](https://xmrchain.net/rawtx) — sends it on.

BestWallet can move the data between the two machines in two ways:

- **Animated QR codes** (Uniform Resources, "UR"), read by webcam
- **Files** carried on a flash drive or SD card

The walkthrough below uses animated QR codes.

### Webcams

Both machines need to scan, so two cheap detachable USB webcams — one each — make life much easier. 720p is plenty. One camera moved back and forth also works if budget is tight.

A laptop's built-in camera can cover one side, but expect to physically hold the machine up to the other screen.

## Initial setup

### 1. Create the offline wallet

![Enable offline mode](static/files/airgap_offline_mode.png)

On the offline device open **Settings → Network → Offline** and tick **Disable all network connections (offline mode)**.

Then [create a new wallet](create-wallet). Write the seed down and store it properly — this is the wallet that holds your money.

The interface will look like this once it opens:

![Offline mode](static/files/airgap_mode.png)

There is no balance and no history here, and that is expected. With no node to talk to, the offline wallet cannot maintain an accurate record, so it keeps none — not even imported outputs. History lives in the view-only wallet.

### 2. Create the matching view-only wallet

The online side needs three things from the offline wallet: the secret view key, the primary address and the restore height. A QR code carries all three at once.

![Press view-only details](static/files/airgap_mode_viewonly.png)

![Show QR](static/files/airgap_transmit.png)

On the **offline** device press **View-only details**, then **Show QR**.

![Restore wallet from keys](static/files/airgap_menu.png)

On the **online** device open the wizard, choose **Restore wallet from keys**, and press **Next**.

![Scan UR](static/files/airgap_restore.png)

Press **Scan QR** and read the code off the offline screen. The dialog closes on its own and fills in the details. If it won't scan, [Help: Airgapped signing issues](airgapped-signing-issues) has fixes.

Finish the wizard:

- Leave the restore height as it is
- Name the wallet whatever you like
- Set a strong password

## Funding it

Take an unused address from the **Receive** tab of the view-only wallet. [Primary address](primary-address) covers the case where you specifically need that one.

If you want to treat the online machine as untrusted, confirm the address against the offline wallet before using it:

1. On the **offline** wallet, click **Show address**.
2. Enter the address index — shown to the left of the address on the view-only wallet's **Receive** tab.
3. Check that the two addresses match.

## Sending

Start a [send](send-transaction) on the view-only wallet as you normally would. Instead of broadcasting, a wizard opens with an animated QR code.

The first transaction from a new wallet also needs key images synchronized. The wizard handles this; it is the reason steps 1 and 2 below exist.

![Click 'Sign a transaction..'](static/files/airgap_mode_sign.png)

On the **offline** wallet, click **Sign a transaction..** to open the scanner:

![Scanning UI](static/files/airgap_scan.png)

1. Camera selector, if you have more than one
2. **Refresh** re-scans for attached cameras
3. Progress fills as fragments are read. Sitting at 99% means a fragment was missed — keep the code in frame.
4. Manual exposure, where the camera supports it. Use the slider when glare is defeating the scan.

The wizard prompts you through each step; the same sequence is written out below.

### Step by step

#### Step 1 — offline wallet scans the outputs

Point the offline wallet at the QR code on the view-only wallet. It advances by itself once complete.

#### Step 2 — view-only wallet scans the key images

Press **Next** on the view-only wallet, then scan the animated code now displayed on the offline wallet.

Expect a brief freeze at 100%; give it a few seconds. The view-only wallet then constructs the transaction. If something fails at this point, just start the send again — steps 1 and 2 do not need repeating.

#### Step 3 — offline wallet scans the unsigned transaction

The view-only wallet displays the constructed transaction as an animated code. Press **Next** on the offline wallet and scan it.

![Sign the transaction](static/files/airgap_sign.png)

The transaction details appear. There are normally two outputs: your change, marked with a yellow or green background, and the recipient.

**This is the moment that matters.** The offline machine is the only one you are trusting — check the destination and amount here, not on the online screen. Then click **Sign**.

#### Step 4 — view-only wallet scans the signed transaction

Press **Next** on the view-only wallet and scan the signed transaction from the offline wallet. Confirm the prompt, and the transaction goes out.

From here on, transactions from this wallet will usually skip steps 1 and 2.
