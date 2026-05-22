#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build [ast-grep](https://github.com/ast-grep/ast-grep) using the Rust
toolchain Chromium ships under `src/third_party/rust-toolchain/`.

Layout mirrors `tools/cr/comby/build_comby.py` and, transitively,
`tools/rust/build_rust.py`:

  * `third_party/ast-grep-src/`                       source clone
  * `third_party/ast-grep-toolchain-intermediate/`    cargo target +
                                                       cargo home (build
                                                       state only)
  * `third_party/ast-grep-toolchain/bin/ast-grep`     final binary

Nothing leaks to `~/.cargo` or any rustup install: `CARGO_HOME` and
`CARGO_TARGET_DIR` are pinned to the intermediate directory, and the
Chromium-bundled `cargo` / `rustc` are placed on `PATH` ahead of
anything the user has globally.
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

# Upstream ast-grep checkout, pinned to a release tag for
# reproducibility. Bump as needed; release notes live at
# https://github.com/ast-grep/ast-grep/releases.
AST_GREP_GIT_URL = 'https://github.com/ast-grep/ast-grep.git'
AST_GREP_REF = '0.42.3'

# Paths used by the build.
_THIRD_PARTY = (brave.root / 'third_party').resolve()
AST_GREP_SRC_DIR: Path = _THIRD_PARTY / 'ast-grep-src'
AST_GREP_TOOLCHAIN_DIR: Path = _THIRD_PARTY / 'ast-grep-toolchain'
AST_GREP_INTERMEDIATE_DIR: Path = (_THIRD_PARTY /
                                   'ast-grep-toolchain-intermediate')
AST_GREP_BIN: Path = AST_GREP_TOOLCHAIN_DIR / 'bin' / 'ast-grep'

# Chromium-bundled Rust toolchain (built by `tools/rust/build_rust.py`
# or fetched via `gclient sync`). On Windows the binaries gain a `.exe`
# suffix; everywhere else they're bare names.
_RUST_EXE = '.exe' if sys.platform == 'win32' else ''
RUST_TOOLCHAIN_DIR: Path = (chromium.root / 'third_party' /
                            'rust-toolchain').resolve()
RUST_TOOLCHAIN_BIN_DIR: Path = RUST_TOOLCHAIN_DIR / 'bin'
CARGO_BIN: Path = RUST_TOOLCHAIN_BIN_DIR / f'cargo{_RUST_EXE}'
RUSTC_BIN: Path = RUST_TOOLCHAIN_BIN_DIR / f'rustc{_RUST_EXE}'


def _check_rust_toolchain() -> None:
    """Fail fast if the Chromium Rust toolchain isn't present.

    The toolchain is normally pulled down by `gclient sync`; if a
    developer is on a custom checkout that doesn't include it, the
    error here points at the recovery path rather than letting the
    cargo invocation produce a less obvious `cargo: command not found`.
    """
    missing = [p for p in (CARGO_BIN, RUSTC_BIN) if not p.is_file()]
    if missing:
        raise RuntimeError(
            f'Chromium Rust toolchain incomplete at {RUST_TOOLCHAIN_DIR}: '
            f'missing {", ".join(str(p) for p in missing)}. Run '
            f'`gclient sync`, or build it locally via '
            f'`tools/rust/build_rust.py`.')


def _cargo_env() -> dict[str, str]:
    """Environment that pins cargo to the Chromium toolchain and keeps
    all of its state inside `AST_GREP_INTERMEDIATE_DIR`.
    """
    env = {**os.environ}
    env['PATH'] = os.pathsep.join(
        [str(RUST_TOOLCHAIN_BIN_DIR),
         env.get('PATH', '')])
    env['RUSTC'] = str(RUSTC_BIN)
    env['CARGO_HOME'] = str(AST_GREP_INTERMEDIATE_DIR / 'cargo-home')
    env['CARGO_TARGET_DIR'] = str(AST_GREP_INTERMEDIATE_DIR / 'target')
    return env


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


def _build_ast_grep(env: dict[str, str], jobs: int) -> None:
    """`cargo build --release --bin ast-grep`, then copy the binary out.

    Uses `cargo build` rather than `cargo install` because ast-grep is
    a Cargo workspace and `cargo install --path .` doesn't accept
    workspace roots; the build-then-copy pattern is also a closer
    analogue to what `build_rust.py` does with `x.py` output.
    """
    AST_GREP_INTERMEDIATE_DIR.mkdir(parents=True, exist_ok=True)
    AST_GREP_TOOLCHAIN_DIR.mkdir(parents=True, exist_ok=True)

    logging.info('Building ast-grep with the Chromium Rust toolchain '
                 '(%s)', RUST_TOOLCHAIN_DIR)
    terminal.run([
        str(CARGO_BIN), 'build', '--release', '--locked', '--bin', 'ast-grep',
        f'--jobs={jobs}'
    ],
                 cwd=AST_GREP_SRC_DIR,
                 env=env,
                 interactive=True)

    built_bin = (Path(env['CARGO_TARGET_DIR']) / 'release' /
                 f'ast-grep{_RUST_EXE}')
    if not built_bin.is_file():
        raise RuntimeError(
            f'cargo build finished but binary not found at {built_bin}')

    AST_GREP_BIN.parent.mkdir(parents=True, exist_ok=True)
    logging.info('Installing %s -> %s', built_bin, AST_GREP_BIN)
    shutil.copy2(built_bin, AST_GREP_BIN)


def _clean() -> None:
    """Remove the source clone and both toolchain directories."""
    for path in (AST_GREP_SRC_DIR, AST_GREP_INTERMEDIATE_DIR,
                 AST_GREP_TOOLCHAIN_DIR):
        if path.exists():
            logging.info('Removing %s', path)
            shutil.rmtree(path)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Build ast-grep with the Chromium Rust toolchain.')
    parser.add_argument(
        '--clean',
        action='store_true',
        help='Remove third_party/ast-grep-src/, '
        'third_party/ast-grep-toolchain/ and '
        'third_party/ast-grep-toolchain-intermediate/ before building.')
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
    _build_ast_grep(_cargo_env(), args.jobs)

    logging.info('Done.')
    logging.info('ast-grep: %s', AST_GREP_BIN)
    return 0


if __name__ == '__main__':
    sys.exit(main())
