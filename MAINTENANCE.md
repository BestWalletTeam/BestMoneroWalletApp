# Maintenance priorities

This document sets out what gets attention first when maintaining BestWallet, and why. It is written for contributors, and for anyone who wants to understand how the project is run.

The ordering matters: items higher up take precedence when time is short.

---

## 1. Security

A security issue affecting BestWallet always justifies a release as soon as a fix exists.

- Fix vulnerabilities and privacy leaks
  — found one? See [SECURITY.md](SECURITY.md) before doing anything public
- Rebase the `monero` submodule onto the newest `monero-project/monero` tag; upstream releases sometimes carry undisclosed security fixes
- Update or patch statically linked dependencies with known vulnerabilities, and read the diff of any package that changes — this is where supply chain attacks land
- Keep compilers and hardening flags current
- Cut the number of third-party dependencies wherever possible
- Improve the [release process](RELEASE.md)

**Longer term**

- Sandbox the components that parse untrusted input, starting with the QR code scanner
- `-static-pie` binaries for Linux targets
- A published feed for security bulletins

## 2. Continuity

The project should survive any individual maintainer losing interest.

- Keep the source repository accessible and mirrored
- Keep release builds straightforward to set up and reproducible over time
- Document release infrastructure and engineering thoroughly enough that someone else could take over

## 3. Reproducibility

[Bootstrappable builds](https://bootstrappable.org/) are required for every release build. The Guix time-machine is pinned to a commit implementing the [Full-Source Bootstrap](https://guix.gnu.org/en/blog/2023/the-full-source-bootstrap-building-from-source-all-the-way-down/).

- Maintain the tooling that detects non-determinism
- Keep releases reproducible, and keep them that way
- Preserve source archives

The build system is documented in [`contrib/guix/README.md`](contrib/guix/README.md).

## 4. Bugs

Fix bugs and crashes. Reports go through the [issue tracker](https://github.com/BestWalletTeam/BestWalletApp/issues).

## 5. Tests

There is no test suite yet beyond what the Monero submodule provides. Building one out is open work, and contributions here are disproportionately valuable.

## 6. Documentation

The in-app guides live in `docs/guides/`; see [HACKING.md](HACKING.md) for how they are built.

- Keep them describing the release that actually ships
- Add troubleshooting guides for problems that recur

**The goal:** most support questions should be answerable with a link to a guide.

## 7. Improvements

Refine what already exists — features, UI and UX alike.

BestWallet is a **wallet** first. Work that makes it a better wallet outranks work that makes it a better anything else.

## 8. Platform support

- Support more architectures and operating systems
- Drop distributions once they reach end of life
- Add hardware wallets as their protocols become supportable

## 9. Housekeeping

- Remove dead code and fix compiler warnings
- Reduce release binary size
- Speed up and automate the release process
- Refactor what needs refactoring; comment what needs comments
- Keep the build system, toolchain and dependencies current
- Remove features whose maintenance cost has outgrown their usefulness

**Ongoing:** stay ready for the migration to [FCMP++](https://www.getmonero.org/2024/04/27/fcmps.html).

## 10. Features

- Support higher, unusual or newly relevant threat models
- Try experimental ideas that the reference wallets may later adopt
- Add things that are broadly useful

Every feature added is a feature that has to be maintained, supported and kept secure indefinitely. Weigh that cost against the benefit before saying yes.

## 11. Upstreaming

Send tried and tested fixes and features upstream. Bugfixes should go up without delay — other people are running the same code.
