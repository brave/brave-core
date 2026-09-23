#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Host linker wrapper that drives `clang` with the hermetic `ld64.lld`.

Used as `CARGO_TARGET_<host>_LINKER` for cargo builds that cross-compile to
another target (e.g. wasm-pack's `wasm32-unknown-unknown` build) while still
building some dependencies (serde, libc, ...) for the macOS host. Without this,
clang's host link step runs `xcrun -find ld` and reaches into the local Xcode
installation instead of the hermetic toolchain.

The obvious fix (passing `--ld-path` via rustflags) doesn't work here:
- `CARGO_TARGET_<host>_RUSTFLAGS` stops applying to build scripts once
  `--target` is set, so the flag never reaches the host units.
- `build.rustflags` does reach them, but also applies to
  `wasm32-unknown-unknown`, where rust-lld rejects `--ld-path` as an unknown
  argument.
- Adding `target.wasm32-unknown-unknown.rustflags` to override it makes cargo
  switch to target-specific rustflags for ALL units, so `build.rustflags` stops
  reaching the host build scripts again - you can't fix both at once.
- `-Z host-config`/`-Z target-applies-to-host` separates host vs. target flags
  cleanly, but it's nightly/unstable.

`CARGO_TARGET_<host>_LINKER` IS applied to host build scripts and is never
passed to the WASM link, so we put `--ld-path` inside the linker invocation
instead of a rustflag. (A plain `build_crate` build has no `--target`, so it
could just set "CARGO_TARGET_..._RUSTFLAGS=-Clink-arg=--ld-path=...";
this wrapper works for both.)
"""

import os
import subprocess
import sys


def main():
    cc = os.environ.get('CC')
    if not cc:
        print('rust_host_linker: $CC is not set', file=sys.stderr)
        return 1
    ld_path = os.path.join(os.path.dirname(cc), 'ld64.lld')
    return subprocess.call([cc, f'--ld-path={ld_path}'] + sys.argv[1:])


if __name__ == '__main__':
    sys.exit(main())
