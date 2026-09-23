# BestWallet

<img width="1302" height="738" alt="BestWallet Monero desktop wallet" src="https://github.com/user-attachments/assets/6a93f48b-9572-49ac-81cb-1192bc6bd9da" />

A free and open-source **Monero desktop wallet and XMR wallet** built for privacy, simplicity, and control. BestWallet runs on Windows, macOS, and Linux and combines a modern interface with advanced Monero features.

### Why BestWallet?

BestWallet is designed to make Monero easier to use without taking away the tools experienced users expect.

* **Modern, approachable interface.** A clean desktop experience designed to make Monero easier to understand, especially for new users.
* **Built-in XMR swaps.** Swap cryptocurrencies to Monero directly inside the wallet without relying on a separate exchange interface.
* **Portfolio tracking.** View your XMR holdings and portfolio information directly from the wallet.
* **Privacy-focused by design.** Built with privacy-conscious users in mind, including Tor support and privacy-focused networking.
* **Advanced Monero controls.** Transaction review, detailed wallet controls, and features for experienced Monero users.
* **Open source.** The source code is publicly available, allowing anyone to inspect, build, and contribute to the project.

BestWallet combines the advanced functionality experienced Monero users expect from wallets such as **Monero GUI and Feather Wallet** with a more streamlined interface, integrated swaps, and portfolio features in one application.

## Privacy & Security

Privacy is central to BestWallet.

BestWallet supports privacy-focused networking and is designed to give users control over how their wallet communicates with the Monero network. Advanced users can configure settings for more specific threat models, while sensible defaults keep the experience straightforward for everyday use.

The in-app documentation covers everything from creating a wallet to advanced topics such as air-gapped signing. Documentation is available through **Help → Documentation**, with its source in [`docs/guides`](https://github.com/BestWalletTeam/BestMoneroWalletApp/tree/main/docs/guides).

## Building

Pre-built releases are available on the [GitHub Releases page](https://github.com/BestWalletTeam/BestMoneroWalletApp/releases), or you can build BestWallet from source.

* **Development builds:** [`HACKING.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/HACKING.md) covers dependencies, CMake options, and development setup.
* **Release builds:** [`contrib/guix/README.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/contrib/guix/README.md) covers the reproducible Guix build used for releases.

```bash
git clone https://github.com/BestWalletTeam/BestMoneroWalletApp.git
cd BestMoneroWalletApp
git submodule update --init --recursive
mkdir build && cd build
cmake .. && cmake --build . -j $(nproc)
```

## Verifying Releases

BestWallet release builds are reproducible and bootstrappable. The build toolchain can itself be built from source, allowing independent builders to verify that published binaries correspond to the tagged source code.

If you did not build BestWallet yourself, verify downloaded releases against the published hashes before running them.

See the [latest release](https://github.com/BestWalletTeam/BestMoneroWalletApp/releases/latest) for available downloads, hashes, and release information.

## Contributing

Issues and pull requests are welcome.

* [`HACKING.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/HACKING.md) — development setup
* [`MAINTENANCE.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/MAINTENANCE.md) — project priorities
* [`RELEASE.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/RELEASE.md) — release process
* [`SECURITY.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/SECURITY.md) — vulnerability reporting

**Do not open a public issue for a security vulnerability.** Please follow the process described in [`SECURITY.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/SECURITY.md).

## Credits

BestWallet builds on the Monero project's `wallet2` implementation and the work of the open-source projects listed in [`THIRD-PARTY-LICENSES.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/THIRD-PARTY-LICENSES.md).

## License

BestWallet's own source code is released under the **BSD-3-Clause license**. See [`LICENSE`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/LICENSE).

Copyright (c) 2020-2026, The Monero Project

BestWallet also includes components under other licenses, including GPL-2.0-or-later and LGPL-3.0. See [`THIRD-PARTY-LICENSES.md`](https://github.com/BestWalletTeam/BestMoneroWalletApp/blob/main/THIRD-PARTY-LICENSES.md) for the complete license information and requirements for distributed binaries.

