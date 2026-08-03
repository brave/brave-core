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

The three builders above share their publishing plumbing (probing whether a
toolchain is already published, writing a sibling YAML index, and uploading the
archive + index pair) via `toolchain_publish.py`, so it stays consistent instead
of being carried as three near-identical copies.
