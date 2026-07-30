# Browser Toolchain Scripts

This directory contains scripts for relating to the browser toolchains
(compilers, linkers, standard libraries, etc.) used by the Brave build.

## Scripts

We have the following scripts to build toolchains that can be used during the
build:

- `build_rust_toolchain.py`: The Rust/WASM toolchain, necessary to build the
  code and the WASM bits.
- `build_xcode_toolchain.py`: The Xcode toolchain, which can be optionally
  provided to build Brave without having to install Xcode.
- `build_windows_toolchain.py`: The Windows toolchain that can be optionally
  used to build Brave without having a Visual Studio install, or to cross-
  compile Brave.

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
the toolchain should be generated, and a new distinct archive is necessary for a
toolchain already in use.
