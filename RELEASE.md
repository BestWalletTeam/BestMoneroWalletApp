# Release process

A release is a tagged commit, built reproducibly with Guix, verified by at least one other person, and published with signed hashes. The steps below are in the order they need to happen.

## 1. Preparation (minor releases)

* Refresh `src/assets/restore_heights_monero_{mainnet,stagenet}.txt`
  — generate the values with `contrib/generate-restore-heights/heights.py`
* Refresh the default node list in `src/assets/nodes.json`
* Bump `openssl`, `qt` and the `tor_*` packages under `contrib/depends/packages`
* Review statically linked dependencies for known vulnerabilities; update or patch anything affected
* Rebase the `monero` submodule onto the latest upstream tag

## 2. Preparation (every release)

* Update `src/assets/ack.txt`
* Update the guides in `docs/guides/` so they describe what is actually shipping
* Set the version in `CMakeLists.txt`
  — a release that carries hardfork support must bump the major version
* Confirm that all `depends` sources are reachable

## 3. Tag

```bash
git tag vX.X.X -a
git push origin vX.X.X
```

Two rules apply here:

* The tag must be annotated and must match the version in `CMakeLists.txt`.
* Only a commit that bumps that version may be tagged.

## 4. Build

Build from a clean clone — never from a working tree that has been used for development.

```bash
git clone https://github.com/BestWalletTeam/BestMoneroWalletApp.git
cd BestMoneroWalletApp
git checkout vX.X.X
git submodule update --init --recursive
./contrib/guix/guix-build
```

## 5. Verify reproducibility

Attest the output with `contrib/guix/guix-attest`.

**Wait for at least one independent party to reproduce the same hashes before going any further.**

If the builds do not match, do not sign and do not publish. Fix the source of the non-determinism, bump the patch version, and start again from the tag step.

## 6. Sign and publish

* Move everything in `guix-build-x.x.x/output` to the signing machine
* Sign the release artifacts and the hash lists
* Publish the tag as a GitHub release, attaching the artifacts, signatures and signed hash lists
* Write the changelog into the release notes; where a release contains security fixes, say so in the title
* Make the `depends` source archives available:

```bash
make -C contrib/depends download
```

## 7. After publishing

Leave a window of up to 7 days between publishing and any wider rollout, so that bug reports from early adopters can arrive before most users update.
