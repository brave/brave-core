#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build [ast-grep](https://github.com/ast-grep/ast-grep) using the Rust
toolchain Chromium ships under `src/third_party/rust-toolchain/`.

The cargo invocation is delegated to Chromium's own wrapper,
`src/tools/crates/run_cargo.py` (the same one `tools/crates/run_gnrt.py`
uses). That wrapper runs the bundled `cargo`/`rustc` and, on Windows, wires in
the hermetic Visual Studio toolchain via `//build/vs_toolchain.py` (linker and
SDK include paths), so none of that has to be reimplemented here.

Layout mirrors `tools/cr/comby/build_comby.py`:

  * `third_party/ast-grep-src/`             source clone
  * `third_party/ast-grep-intermediate/`    cargo target dir + cargo home
                                            (build state only)
  * `third_party/ast-grep/bin/ast-grep`     final binary

Nothing leaks to `~/.cargo` or any rustup install: `run_cargo` pins
`CARGO_HOME` and prepends the bundled toolchain to `PATH`, and the cargo
`--target-dir` is pinned to the intermediate directory.
"""

from __future__ import annotations

import argparse
import logging
import os
import shutil
import sys
from pathlib import Path

# Make sibling `tools/cr` modules importable when this script is run
# directly from its subdirectory.
_CR_DIR = str(Path(__file__).resolve().parent.parent)
if _CR_DIR not in sys.path:
    sys.path.insert(0, _CR_DIR)

from repository import brave, chromium  # noqa: E402
from terminal import terminal  # noqa: E402

# Reuse Chromium's cargo wrapper rather than reimplementing toolchain and
# Windows VS-toolchain handling. `RunCargo` runs the bundled toolchain pinned
# by `DEFAULT_SYSROOT` (== src/third_party/rust-toolchain).
sys.path.insert(0, str((chromium.root / 'tools' / 'crates').resolve()))
from run_cargo import DEFAULT_SYSROOT, RunCargo  # noqa: E402

# Upstream ast-grep checkout, pinned to a release tag for
# reproducibility. Bump as needed; release notes live at
# https://github.com/ast-grep/ast-grep/releases.
AST_GREP_GIT_URL = 'https://github.com/ast-grep/ast-grep.git'
AST_GREP_REF = '0.44.0'

# Paths used by the build.
_THIRD_PARTY = (brave.root / 'third_party').resolve()
AST_GREP_SRC_DIR: Path = _THIRD_PARTY / 'ast-grep-src'
AST_GREP_DIR: Path = _THIRD_PARTY / 'ast-grep'
AST_GREP_INTERMEDIATE_DIR: Path = (_THIRD_PARTY / 'ast-grep-intermediate')

# On Windows the cargo-built binary gains a `.exe` suffix; everywhere else it's
# a bare name. Kept in sync with plaster.AST_GREP_BIN.
_RUST_EXE = '.exe' if sys.platform == 'win32' else ''
AST_GREP_BIN: Path = AST_GREP_DIR / 'bin' / f'ast-grep{_RUST_EXE}'


def _check_rust_toolchain() -> None:
    """Fail fast if the Chromium Rust toolchain isn't present.

    The toolchain is normally pulled down by `gclient sync`; if a
    developer is on a custom checkout that doesn't include it, the
    error here points at the recovery path rather than letting the
    cargo invocation produce a less obvious `cargo: command not found`.
    """
    cargo = DEFAULT_SYSROOT / 'bin' / f'cargo{_RUST_EXE}'
    if not cargo.is_file():
        raise RuntimeError(
            f'Chromium Rust toolchain incomplete at {DEFAULT_SYSROOT}: '
            f'missing {cargo}. Run `gclient sync`, or build it locally via '
            f'`tools/rust/build_rust.py`.')


def _clone_ast_grep() -> None:
    """Shallow-clone ast-grep if not already present.

    Pre-existing checkouts are left alone so local edits / a custom
    branch survive across runs. `--clean` wipes and re-clones.
    """
    if AST_GREP_SRC_DIR.is_dir():
        logging.info('ast-grep source already present at %s', AST_GREP_SRC_DIR)
        return

    AST_GREP_SRC_DIR.parent.mkdir(parents=True, exist_ok=True)
    logging.info('Cloning ast-grep (%s) into %s', AST_GREP_REF,
                 AST_GREP_SRC_DIR)
    terminal.run([
        'git', 'clone', '--depth=1', '--branch', AST_GREP_REF,
        AST_GREP_GIT_URL,
        str(AST_GREP_SRC_DIR)
    ],
                 interactive=True)


def _build_ast_grep(jobs: int) -> None:
    """Build the `ast-grep` binary, then copy it into place.

    Delegates the cargo invocation to `run_cargo.RunCargo` (mirroring
    `tools/crates/run_gnrt.py`) so the bundled toolchain -- and on Windows the
    hermetic VS toolchain flags -- are applied. All cargo state is pinned under
    the intermediate dir: the target dir via `--target-dir` and the cargo home
    via `RunCargo`'s `home_dir`. cargo runs from the source checkout so its
    `.cargo/config.toml` is honoured.

    `cargo build` is used (rather than `cargo install`) because ast-grep is a
    Cargo workspace and `cargo install --path .` rejects workspace roots; the
    build-then-copy pattern also mirrors what `build_rust.py` does with `x.py`.
    """
    AST_GREP_INTERMEDIATE_DIR.mkdir(parents=True, exist_ok=True)
    AST_GREP_DIR.mkdir(parents=True, exist_ok=True)

    target_dir = AST_GREP_INTERMEDIATE_DIR / 'target'
    home_dir = AST_GREP_INTERMEDIATE_DIR / 'cargo-home'

    logging.info('Building ast-grep with the Chromium Rust toolchain (%s)',
                 DEFAULT_SYSROOT)
    cargo_args = [
        '--locked', 'build', '--release', '--bin', 'ast-grep', '--target-dir',
        str(target_dir), f'--jobs={jobs}'
    ]
    # Run cargo from the source checkout so its `.cargo/config.toml` is honoured
    # (RunCargo has no cwd parameter). `contextlib.chdir` would be cleaner but
    # is unknown to our pinned pylint.
    prev_cwd = Path.cwd()
    os.chdir(AST_GREP_SRC_DIR)
    try:
        returncode = RunCargo(DEFAULT_SYSROOT, str(home_dir), cargo_args)
    finally:
        os.chdir(prev_cwd)
    if returncode != 0:
        raise RuntimeError(f'cargo build failed (exit {returncode})')

    built_bin = target_dir / 'release' / f'ast-grep{_RUST_EXE}'
    if not built_bin.is_file():
        raise RuntimeError(
            f'cargo build finished but binary not found at {built_bin}')

    AST_GREP_BIN.parent.mkdir(parents=True, exist_ok=True)
    logging.info('Installing %s -> %s', built_bin, AST_GREP_BIN)
    shutil.copy2(built_bin, AST_GREP_BIN)


def _clean() -> None:
    """Remove the source clone and both toolchain directories."""
    for path in (AST_GREP_SRC_DIR, AST_GREP_INTERMEDIATE_DIR, AST_GREP_DIR):
        if path.exists():
            logging.info('Removing %s', path)
            shutil.rmtree(path)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Build ast-grep with the Chromium Rust toolchain.')
    parser.add_argument('--clean',
                        action='store_true',
                        help='Remove third_party/ast-grep-src/, '
                        'third_party/ast-grep/ and '
                        'third_party/ast-grep-intermediate/ before building.')
    parser.add_argument('-j',
                        '--jobs',
                        type=int,
                        default=os.cpu_count() or 1,
                        help='Number of parallel build jobs (default: nproc).')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable debug logging.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        force=True)

    if args.clean:
        _clean()

    _check_rust_toolchain()
    _clone_ast_grep()
    _build_ast_grep(args.jobs)

    logging.info('Done.')
    logging.info('ast-grep: %s', AST_GREP_BIN)
    return 0


if __name__ == '__main__':
    sys.exit(main())
