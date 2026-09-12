# BestWallet

A free and open-source Monero desktop wallet, written in C++ and Qt6. It runs on Windows, macOS, and Linux.

- **Small and quick.** Comfortable on ordinary hardware, including virtual machines and live systems.
- **Approachable, without holding you back.** Sensible for a first Monero wallet, while offering the feature set an experienced user would otherwise go to the CLI for.
- **Sane out of the box, configurable when you need it.** Defaults suit most people; the settings are there for higher or more unusual threat models.
- **Somewhere to try things.** Experimental features can be exercised here before they are ready for the reference wallets.

The in-app help browser (**Help → Documentation**) covers everything from creating a wallet to airgapped signing. Its source lives in [`docs/guides`](docs/guides).

## Building

There are no binary releases yet — build from source.

- **Development builds:** [HACKING.md](HACKING.md) covers dependencies, CMake options and IDE setup.
- **Release builds:** [contrib/guix/README.md](contrib/guix/README.md) covers the reproducible, bootstrappable Guix build used for releases.

In short:

```bash
git clone https://github.com/BestWalletTeam/BestWalletApp.git
cd BestWallet
git submodule update --init --recursive
mkdir build && cd build
cmake .. && cmake --build . -j $(nproc)
```

Release builds target x86_64 and aarch64 Linux and x86_64 Windows.

## Verifying what you run

Release builds are reproducible and bootstrappable: the toolchain itself can be built from source rather than trusted as a binary download, and independent builders can confirm that a published artifact matches the tagged source. If you did not build it yourself, verify it against the published hashes before running it.

## Contributing

Issues and pull requests are welcome at [github.com/BestWalletTeam/BestWalletApp](https://github.com/BestWalletTeam/BestWalletApp).

- [HACKING.md](HACKING.md) — setting up a development environment
- [MAINTENANCE.md](MAINTENANCE.md) — what the project prioritises, and why
- [RELEASE.md](RELEASE.md) — how a release is cut
- [SECURITY.md](SECURITY.md) — reporting a vulnerability. **Do not open a public issue for a security problem.**

## Credits

BestWallet builds on the Monero project's `wallet2` implementation, and on the work of the open-source projects listed in [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md). They are why this exists at all.

## License

This project's own source is released under the BSD-3-Clause license. See [LICENSE](LICENSE).

Copyright (c) 2020-2026, The Monero Project

BestWallet also links components under other licenses, including GPL-2.0-or-later
(KeePassXC-derived QR and async helpers) and LGPL-3.0 (Qt, polyseed). Because the
GPL components are compiled in, a **distributed binary must be offered under
GPL-2.0-or-later terms**, with complete corresponding source available. See
[THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md) for the full breakdown and the
static-linking obligations that apply to release builds.
