# Security Policy

## Reporting a vulnerability

**Do not open a public issue for a security problem.**

Report it privately through GitHub Security Advisories:

**[github.com/BestWalletTeam/BestWalletApp/security/advisories/new](https://github.com/BestWalletTeam/BestWalletApp/security/advisories/new)**

That channel is private between you and the maintainers, and keeps a record of the disclosure timeline.

### What to include

- Which version or commit you tested, and on which platform
- What an attacker gains — be concrete about the impact
- Steps to reproduce, and a proof of concept where you have one
- Whether the issue is already public anywhere

### What to expect

The report will be acknowledged, and you will be told whether it is accepted and roughly when a fix is expected. Please give the maintainers a reasonable window to ship that fix before discussing the issue publicly. Credit is given in the release notes unless you would rather stay anonymous.

## Scope

In scope is the code in this repository, at the latest tagged release.

The following are **out of scope** here, though they are still worth reporting to whoever does own them:

- **The Monero submodule.** Anything not introduced by patches in this repository belongs [upstream](https://github.com/monero-project/meta/blob/master/VULNERABILITY_RESPONSE_PROCESS.md).
- **Third-party dependencies.** Report these to the project that maintains them.
- **Hardware wallets.** Firmware and device issues go to the device vendor.
- **Malware on the user's machine**, including clipboard hijackers and keyloggers.
- **Memory imaging**, cold boot attacks and similar physical attacks against a running system.
- **Social engineering** of users or maintainers, and coercion of any kind.
- **Issues already fixed in `master`** at the time the report arrives.

Custom builds and distribution packages are also out of scope — an issue needs to be reproducible in a build produced from this repository.

## No bug bounty

There is no bug bounty program and no monetary reward. Reports are still very welcome, and handled seriously.
