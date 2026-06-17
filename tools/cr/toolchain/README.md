# Browser Toolchain Scripts

This directory contains scripts for generating browser toolchains (compilers,
linkers, standard libraries, etc.) used by the Brave build.

## Design rules

Scripts here are designed to be launched with a single `curl` one-liner,
so they must be **self-contained in a single source file** with **no
dependencies outside the Python standard library** (or the platform's
default tooling such as `git`). Do not split logic across helper modules or
add third-party imports.

Example invocation pattern:

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchain/<script>.py \
    | python3 - [args...]
```

## Scripts

### `build_rust_toolchain.py`

Builds and packages a minimal Rust toolchain subset for Chromium: the
`rust-lld` linker and the `wasm32-unknown-unknown` stage-1 standard-library
sysroot. The output is a `.tar.xz` archive ready for upload to GCS.

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchain/build_rust_toolchain.py \
    | python3 - \
        --out-dir=./out/ \
        --chromium-src=~/dev/chromium/src/
```

Pass `--help` for the full list of options.
