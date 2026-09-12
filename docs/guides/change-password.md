---
title: How to change the wallet password
nav_title: Change wallet password
category: howto
---

Before you touch the password, confirm that your [seed](show-wallet-seed) is written down somewhere safe. The password protects the files on disk; the seed is what actually recovers the wallet. Lose both and the funds are gone.

**Changing it**

Open **Wallet → Password**, or click the lock icon in the statusbar.

Use something long and random from a password manager — [KeePassXC](https://keepassxc.org/) is a good choice. Once you confirm, your [wallet files](wallet-files) are re-encrypted under the new password.

### If the old password was weak or absent

Re-encrypting writes new files; it does not scrub the old ones. On most systems the previous bytes [remain on the storage device](https://unix.stackexchange.com/a/593340) and can be recovered afterwards through [filesystem metadata](http://manpages.ubuntu.com/manpages/bionic/man8/ext4magic.8.html), [file carving](https://en.wikipedia.org/wiki/File_carving), [snapshots](https://en.wikipedia.org/wiki/Snapshot_(computer_storage)) and similar means.

So if the wallet was previously unencrypted or protected by something guessable, changing the password does not retroactively secure what is already on disk. Whether that matters depends on your threat model — but where it does, the safer course is to move the funds into a freshly created wallet.

How recoverable those old bytes are comes down to your filesystem, drive and operating system. There is no general answer, and this is outside what the BestWallet developers can help with.
