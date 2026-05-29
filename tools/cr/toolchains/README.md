# Browser Toolchain Scripts

This directory contains scripts for relating to the browser toolchains (compilers,
linkers, standard libraries, etc.) used by the Brave build.

## Design rules

Scripts here are designed to be launched with a single `curl` one-liner,
so they must be **self-contained in a single source file** with **no
dependencies outside the Python standard library**. Do not split logic across
helper modules or add third-party imports.

Example invocation pattern:

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/<script>.py \
    | python3 - [args...]
```

## Scripts

### `bootstrap_depot_tools.py`

Bootstraps `depot_tools` on a fresh CI worker, and then runs a given python
script using `vpython3`. This allows for the toolchain python scripts to be run
across different environments with the same guarantees. Arguments after `--`
are forwarded verbatim.

```sh
curl -sSLf \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/bootstrap_depot_tools.py \
    | python3 - \
        --run=https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/build_rust_toolchain.py \
        -- \
        --out-dir=./out/ --chromium-src=chromium/src --clone-chromium \
        --use-ref=150.0.7850.1
```

### `build_rust_toolchain.py`

Builds and packages a minimal Rust toolchain subset for Chromium: the
`rust-lld` linker and the `wasm32-unknown-unknown` stage-1 standard-library
sysroot. The output is a `.tar.xz` archive ready for upload to GCS.

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/build_rust_toolchain.py \
    | python3 - \
        --out-dir=./out/ \
        --chromium-src=~/dev/chromium/src/
```

Pass `--help` for the full list of options.

### Reproducibility

Reproducibility of the rust toolchain is important, and the builder we have has
the necessary constrols to make sure we have enough information to generate the
same toolchain again if necessary.

Another aspect of reproducibility for the builder is to make sure we are able to
verify that a toolchain used is distinct, and that there is no chance for an
archived toolchain to be replaced. For this reason, the builder also maintains
and index for a given toolchain signature. This archive index can be used by the
client to query what toolchains we have, and to pin any given toolchain with a
hashsum value during checkout.

The index aggregates every build for the same platform and upstream Rust+Clang
combination, and is the canonical source for "what toolchains are available."
Each entry stores all the relevant information for all the necessary elements
that led to the creation of the toolchain.

`--brave-subrevision=<int>` exists as the lever for minting a distinct new
toolchain at the same upstream Rust+Clang revisions. The script encodes it as
the final section of the tarball filename, so bumping it produces a fresh URL.
This control is important for cases where there are changes on our end for how
the toolchain should be generated, and a new distinct archive is necessary for
a toolchain already in use.

### `build_xcode_toolchain.py`

macOS-only. Builds a hermetic, reproducible Xcode toolchain archive from
the local Xcode.app installation. The archive contains the subset of
files listed in Chromium's `build/xcode_binaries.yaml` plus the on-demand
Metal toolchain, with all archive metadata zeroed for reproducible builds.

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/build_xcode_toolchain.py \
    | python3 - \
        --out-dir=./out/ \
        --chromium-tag=150.0.7841.1
```

The output filename encodes the Xcode and SDK versions and is consumed by
`brockit update-xcode-toolchain` to pin Brave's
`build/mac/download_hermetic_xcode.py`. Pass `--help` for the full list of
options; see the script's module docstring for the archive filename format.

### `xcode_accept_license.py`

macOS-only. Writes the two preference keys Apple's command-line tools
check for license acceptance
(`IDEXcodeVersionForAgreedToGMLicense` and `IDELastGMLicenseAgreedTo`)
into `/Library/Preferences/com.apple.dt.Xcode.plist`. Designed to be
installed at `/usr/local/bin/` and invoked under a narrowly-scoped
NOPASSWD sudoers grant so build hooks
(`brave/build/mac/download_hermetic_xcode.py`) can accept the license
without a password prompt.

The script's own docstring covers manual install, the corresponding
sudoers entry, and the security model. Run with `--help` for the CLI.

### `install_xcode_accept_license.py`

Installer/verifier for the helper above. Lays down both the script
(`/usr/local/bin/xcode_accept_license.py`, mode 0755 owned by
`root:wheel`) and the matching sudoers drop-in
(`/etc/sudoers.d/xcode_accept_license`, mode 0440 owned by root,
validated via `visudo -c`), then runs a non-destructive smoke test using
`sudo -n -l` to confirm the grant works without ever invoking the helper.

```sh
# install + verify
python3 install_xcode_accept_license.py --username $USER

# verify an existing install
python3 install_xcode_accept_license.py --check-only --username $USER

# tear down
python3 install_xcode_accept_license.py --uninstall
```

The script re-execs under sudo automatically if not invoked as root, so
there's no need to remember the `sudo` prefix.
