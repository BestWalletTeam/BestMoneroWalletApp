# Developer guide

BestWallet is a Qt6 desktop application written in C++. Development happens mainly on Linux; macOS works as well. Building on Windows is not supported at present.

Rolling-release distributions and the current Ubuntu release are what we test against. Older stable distributions may work, but nothing is guaranteed.

The minimum supported Qt version is **6.4**.

## Getting the source

```bash
git clone https://github.com/BestWalletTeam/BestMoneroWalletApp.git
cd BestMoneroWalletApp
git submodule update --init --recursive
```

The `monero` submodule is large — expect the initial clone to take a while.

## Dependencies

Pick the section matching your system.

### Arch Linux

```bash
pacman -S git cmake base-devel ccache unbound boost qrencode qt6-base qt6-svg qt6-websockets qt6-wayland qt6-multimedia qt6-charts libzip hidapi protobuf zxing-cpp
```

### Ubuntu 24.04

```bash
apt update
apt install git cmake build-essential ccache libssl-dev libunbound-dev libboost-all-dev \
            libqrencode-dev qt6-base-dev qt6-svg-dev qt6-websockets-dev qt6-multimedia-dev \
            qt6-wayland-dev libqt6charts6-dev libzip-dev libsodium-dev libgcrypt20-dev \
            libx11-xcb-dev libprotobuf-dev libhidapi-dev libzxing-dev libusb-1.0-0-dev
```

### RHEL 9 / AlmaLinux / Rocky

```bash
sudo dnf install epel-release -y
sudo dnf groupinstall "Development Tools" -y
sudo dnf install unbound-devel boost-devel qrencode-devel zxing-cpp-devel qt6-qtbase-devel \
                 qt6-qtsvg-devel qt6-qtwebsockets-devel qt6-qtmultimedia-devel \
                 qt6-qtwayland-devel qt6-qtcharts-devel libsodium-devel
```

### Void Linux

```bash
xbps-install -S base-devel cmake boost-devel openssl-devel unbound-devel libsodium-devel zlib-devel qt6-base-devel \
                qt6-svg-devel qt6-websockets-devel qt6-multimedia-devel qt6-wayland-devel qt6-charts-devel \
                libgcrypt-devel libzip-devel hidapi-devel protobuf protobuf-devel qrencode-devel zxing-cpp-devel
```

### macOS

[Homebrew](https://brew.sh) is the path of least resistance:

```bash
brew install qt libsodium libzip qrencode unbound cmake boost hidapi openssl expat libunwind-headers protobuf pkgconfig
```

`zxing-cpp` is not packaged; either build [it](https://github.com/zxing-cpp/zxing-cpp) from source or configure with `-DWITH_SCANNER=Off`.

## Tor

Reaching `.onion` nodes and the data service requires a Tor daemon. Every build embeds the in-tree binary at `src/assets/tor` by default, which the wallet unpacks into its config directory and runs on `127.0.0.1:19450`. That binary is a static linux-x86_64 build; other targets need `-DTOR_DIR=/path/to/tor` (with `-DTOR_VERSION`) to bundle one, and `-DTOR_BUNDLED=Off` builds without Tor at all.

Running Tor as a system service is the better option day to day: the wallet then attaches to the existing daemon instead of spawning and managing a child process on every launch.

```bash
# Arch
pacman -S tor && systemctl enable --now tor

# Debian / Ubuntu
apt update && apt install tor && systemctl enable --now tor

# Void
xbps-install tor && ln -s /etc/sv/tor /var/service/. && sv start tor

# macOS
brew install tor && brew services restart tor
```

## Building

### From the command line

```bash
mkdir build
cd build
cmake ..
cmake --build . -j $(nproc)
```

Where `execinfo.h` is unavailable, configure with `-DSTACK_TRACE:BOOL=OFF`.

The binary is named `BestWallet`, matching the CMake target.

### With CLion

CLion handles the CMake integration and ships a usable debugger, which makes it a comfortable environment for this codebase.

1. `File → Settings → Build → CMake` — set the build type to `Debug` and add any options you want.
2. Open the CMake tool window (`View → Tool Windows → CMake`) and press **Reload CMake Project**.
3. `Run → Edit configurations` — select the `BestWallet` target.

Two things worth setting on that run configuration:

* `MONERO_LOG_LEVEL=1` in the environment, for verbose logging
* `--stagenet` in the program arguments, to work against stagenet instead of mainnet

Then `Run → Run 'BestWallet'`, or Shift + F10.

## Editing `.ui` files

Qt Designer 6.7 and later writes fully-scoped enum names into `.ui` files
(`Qt::Orientation::Vertical` rather than `Qt::Vertical`). `uic` before 6.7 compares
those strings literally, does not recognise the scoped form, and silently generates
spacers with their two `QSizePolicy` arguments swapped -- the build still succeeds,
but layouts come out wrong on older Qt.

Keep `.ui` enums in the unscoped form. Every Qt 6 `uic` understands it. After editing
a form in Designer, run:

```bash
scripts/check-ui-enums.sh --fix
```

CMake fails at configure time if a scoped enum makes it into a `.ui` file.

## CMake options

| Option                     | Default | Effect                                                                                                          |
| -------------------------- | ------- | --------------------------------------------------------------------------------------------------------------- |
| `-DSTATIC=ON`              | OFF     | Link statically. Requires a static Qt.                                                                          |
| `-DSELF_CONTAINED=OFF`     | OFF     | Turn off when building for distribution packages                                                                |
| `-DTOR_DIR=/path/to/tor/`  | OFF     | Embed Tor binaries from the given directory instead of the in-tree one. `TOR_VERSION` must be set alongside it. |
| `-DTOR_BUNDLED=OFF`        | ON      | Do not embed any Tor binary. The wallet then expects a system Tor on `socks5Host:socks5Port`.                   |
| `-DCHECK_UPDATES=ON`       | OFF     | Build the update checker. Standalone binaries only.                                                             |
| `-DPLATFORM_INSTALLER=ON`  | OFF     | Updater fetches an installer rather than an archive (Windows only)                                              |
| `-DUSE_DEVICE_TREZOR=OFF`  | ON      | Drop Trezor hardware wallet support                                                                             |
| `-DWITH_SCANNER=OFF`       | ON      | Drop the webcam QR code scanner                                                                                 |
| `-DSTACK_TRACE=ON`         | OFF     | Dump a stack trace on crash (Linux only)                                                                        |
| `-DWITH_PLUGIN_<NAME>=OFF` | ON      | Exclude a plugin: `HOME`, `TICKERS`, `CROWDFUNDING`, `REVUO`, `CALC`                                            |

## Documentation

The in-app help browser is built from `docs/guides/`. Each guide is a Markdown file with YAML frontmatter supplying `title`, `nav_title` and `category`; only the categories listed in `contrib/docs/generate.py` are shipped in the application.

CMake regenerates `src/assets/docs/` and `src/assets_docs.qrc` from that directory on every configure, so edit `docs/guides/` and never the generated output.

To regenerate by hand:

```bash
python3 contrib/docs/generate.py
```

## Reproducible builds

Release builds are produced with Guix and are bootstrappable. See [`contrib/guix/README.md`](contrib/guix/README.md).
