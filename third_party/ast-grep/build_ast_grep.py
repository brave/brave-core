#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build [ast-grep](https://github.com/ast-grep/ast-grep) using the Rust
toolchain Chromium ships under `src/third_party/rust-toolchain/`, then compile
the `gn` custom-language grammar with Chromium's bundled clang/lld.

"""

from __future__ import annotations

import argparse
import logging
import os
import shutil
import subprocess
import sys
from pathlib import Path

import build_utils
from build_utils import AST_GREP_DIR, AST_GREP_PLATFORM_DIR, CHROMIUM_ROOT, \
    THIRD_PARTY

# We reuse Chromium's cargo wrapper.
sys.path.insert(0, str((CHROMIUM_ROOT / 'tools' / 'crates').resolve()))
from run_cargo import DEFAULT_SYSROOT, RunCargo  # noqa: E402

# The `gn` custom-language grammar (GN has no built-in ast-grep grammar) is
# built by a sibling script, since it uses a different toolchain (Chromium's
# clang/lld) than ast-grep itself (Chromium's Rust).
import build_tree_sitter_gn  # noqa: E402  pylint: disable=wrong-import-position

# ast-grep upstream details.
AST_GREP_GIT_URL = 'https://github.com/ast-grep/ast-grep.git'

# Pulling this specific hash for `0.45.3`.
AST_GREP_REF = '979c143639e3588c31f563091e69398683ed34f0'

# Additional paths under `third_party/` for the source checkout, intermediate
# build state, and final binary output.
AST_GREP_SRC_DIR: Path = THIRD_PARTY / 'ast-grep-src'
AST_GREP_INTERMEDIATE_DIR: Path = (THIRD_PARTY / 'ast-grep-intermediate')

_RUST_EXE = '.exe' if sys.platform == 'win32' else ''

AST_GREP_BIN: Path = AST_GREP_PLATFORM_DIR / 'bin' / f'ast-grep{_RUST_EXE}'

# Third-party cargo subcommands (cargo-audit) are installed here with the
# bundled toolchain, then resolved from this `bin/` on PATH.
_CARGO_TOOLS_ROOT: Path = AST_GREP_INTERMEDIATE_DIR / 'cargo-tools'
_CARGO_TOOLS_BIN: Path = _CARGO_TOOLS_ROOT / 'bin'


def _check_rust_toolchain() -> None:
    """Fail fast if the Chromium Rust toolchain isn't present.
    """
    cargo = DEFAULT_SYSROOT / 'bin' / f'cargo{_RUST_EXE}'
    if not cargo.is_file():
        raise RuntimeError(
            f'Chromium Rust toolchain incomplete at {DEFAULT_SYSROOT}: '
            f'missing {cargo}. Run `gclient sync`, or build it locally via '
            f'`tools/rust/build_rust.py`.')


def _clone_ast_grep() -> None:
    """Shallow-fetch ast-grep at `AST_GREP_REF` if not already present.

    `--clean` wipes `AST_GREP_SRC_DIR` first to force a re-fetch.
    """
    build_utils.shallow_clone_pinned(AST_GREP_GIT_URL, AST_GREP_REF,
                                     AST_GREP_SRC_DIR)


def _run_cargo_in_src(cargo_args: list[str]) -> int:
    """Run cargo from the ast-grep source checkout, returning the exit code.

    Runs cargo via `RunCargo` (bundled toolchain) from `AST_GREP_SRC_DIR` so the
    checkout's `.cargo/config.toml` is honoured, with the installed
    cargo-subcommand `bin/` on `PATH` so external subcommands (cargo-audit)
    resolve. `RunCargo` has no cwd/env parameters, hence the manual
    save/restore.
    """
    home_dir = AST_GREP_INTERMEDIATE_DIR / 'cargo-home'
    prev_cwd = Path.cwd()
    prev_path = os.environ.get('PATH', '')
    os.environ['PATH'] = os.pathsep.join([str(_CARGO_TOOLS_BIN), prev_path])
    os.chdir(AST_GREP_SRC_DIR)
    try:
        return RunCargo(DEFAULT_SYSROOT, str(home_dir), cargo_args)
    finally:
        os.chdir(prev_cwd)
        os.environ['PATH'] = prev_path


def _install_cargo_tool(tool: str) -> None:
    """Install a third-party cargo subcommand with the bundled toolchain.

    Installs into `_CARGO_TOOLS_ROOT` (shared `bin/`), skipping the build when
    the tool is already present. `_run_cargo_in_src` puts that `bin/` on `PATH`
    so cargo resolves the subcommand.
    """
    if (_CARGO_TOOLS_BIN / f'{tool}{_RUST_EXE}').is_file():
        return
    home_dir = AST_GREP_INTERMEDIATE_DIR / 'cargo-home'
    logging.info('Installing %s into %s', tool, _CARGO_TOOLS_ROOT)
    returncode = RunCargo(
        DEFAULT_SYSROOT, str(home_dir),
        ['install', tool, '--locked', '--root',
         str(_CARGO_TOOLS_ROOT)])
    if returncode != 0:
        raise RuntimeError(f'cargo install {tool} failed (exit {returncode})')


def _build_ast_grep(jobs: int) -> None:
    """Build the `ast-grep` binary, then copy it into place.
    """
    AST_GREP_INTERMEDIATE_DIR.mkdir(parents=True, exist_ok=True)
    AST_GREP_PLATFORM_DIR.mkdir(parents=True, exist_ok=True)

    target_dir = AST_GREP_INTERMEDIATE_DIR / 'target'
    logging.info('Building ast-grep with the Chromium Rust toolchain (%s)',
                 DEFAULT_SYSROOT)
    returncode = _run_cargo_in_src([
        'build', '--locked', '--release', '--bin', 'ast-grep', '--target-dir',
        str(target_dir), f'--jobs={jobs}'
    ])
    if returncode != 0:
        raise RuntimeError(f'cargo build failed (exit {returncode})')

    built_bin = target_dir / 'release' / f'ast-grep{_RUST_EXE}'
    if not built_bin.is_file():
        raise RuntimeError(
            f'cargo build finished but binary not found at {built_bin}')

    AST_GREP_BIN.parent.mkdir(parents=True, exist_ok=True)
    logging.info('Installing %s -> %s', built_bin, AST_GREP_BIN)
    shutil.copy2(built_bin, AST_GREP_BIN)


def _audit_ast_grep() -> None:
    """Audit the locked dependency graph against the RustSec advisory database.
    """
    _install_cargo_tool('cargo-audit')
    logging.info('Auditing ast-grep dependencies with cargo audit')
    returncode = _run_cargo_in_src(['audit'])
    if returncode != 0:
        raise RuntimeError(f'cargo audit reported issues (exit {returncode})')


def _clean() -> None:
    """Remove the source clone, intermediate build state, and binary output.
    """
    paths = [AST_GREP_SRC_DIR, AST_GREP_INTERMEDIATE_DIR]
    if AST_GREP_DIR.is_dir():
        paths += [p for p in AST_GREP_DIR.glob('ast-grep-*') if p.is_dir()]
    for path in paths:
        if path.exists():
            logging.info('Removing %s', path)
            shutil.rmtree(path)


def build(jobs: int, clean: bool = False) -> None:
    """Audit and build ast-grep, then compile the `gn` custom-language grammar.

    Everything lands under `third_party/ast-grep/ast-grep-<os>/`.
    """
    if clean:
        _clean()
    _check_rust_toolchain()
    _clone_ast_grep()
    _audit_ast_grep()
    _build_ast_grep(jobs)
    build_tree_sitter_gn.build(clean=clean)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Build ast-grep with the Chromium Rust toolchain.')
    parser.add_argument('--clean',
                        action='store_true',
                        help='Remove third_party/ast-grep-src/, '
                        'third_party/tree-sitter-gn-src/, '
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

    build(args.jobs, clean=args.clean)

    logging.info('Done.')
    logging.info('ast-grep: %s', AST_GREP_BIN)
    logging.info('gn sgconfig: %s', AST_GREP_PLATFORM_DIR / 'sgconfig.yml')
    return 0


if __name__ == '__main__':
    sys.exit(main())
