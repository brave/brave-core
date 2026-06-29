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
import subprocess
import sys
from pathlib import Path

# Roots derived from this file's location: brave-core is three directories up
# (`tools/cr/ast_grep` -> brave) and Chromium's `src/` is its parent. Deriving
# them from `__file__` keeps this script self-contained -- it imports no sibling
# `tools/cr` modules, so a consumer can sparse-checkout just `tools/cr/ast_grep`
# and still build.
_BRAVE_ROOT = Path(__file__).resolve().parents[3]
_CHROMIUM_ROOT = _BRAVE_ROOT.parent

# Reuse Chromium's cargo wrapper rather than reimplementing toolchain and
# Windows VS-toolchain handling. `RunCargo` runs the bundled toolchain pinned
# by `DEFAULT_SYSROOT` (== src/third_party/rust-toolchain).
sys.path.insert(0, str((_CHROMIUM_ROOT / 'tools' / 'crates').resolve()))
from run_cargo import DEFAULT_SYSROOT, RunCargo  # noqa: E402

# Upstream ast-grep checkout, pinned to a release tag for
# reproducibility. Bump as needed; release notes live at
# https://github.com/ast-grep/ast-grep/releases.
AST_GREP_GIT_URL = 'https://github.com/ast-grep/ast-grep.git'
AST_GREP_REF = '0.44.0'

# Paths used by the build.
_THIRD_PARTY = _BRAVE_ROOT / 'third_party'
AST_GREP_SRC_DIR: Path = _THIRD_PARTY / 'ast-grep-src'
AST_GREP_DIR: Path = _THIRD_PARTY / 'ast-grep'
AST_GREP_INTERMEDIATE_DIR: Path = (_THIRD_PARTY / 'ast-grep-intermediate')

_RUST_EXE = '.exe' if sys.platform == 'win32' else ''
AST_GREP_BIN: Path = AST_GREP_DIR / 'bin' / f'ast-grep{_RUST_EXE}'

# Third-party cargo subcommands (cargo-auditable, cargo-audit) are installed
# here with the bundled toolchain, then resolved from this `bin/` on PATH.
_CARGO_TOOLS_ROOT: Path = AST_GREP_INTERMEDIATE_DIR / 'cargo-tools'
_CARGO_TOOLS_BIN: Path = _CARGO_TOOLS_ROOT / 'bin'


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
    subprocess.run([
        'git', 'clone', '--depth=1', '--branch', AST_GREP_REF,
        AST_GREP_GIT_URL,
        str(AST_GREP_SRC_DIR)
    ],
                   check=True)


def _run_cargo_in_src(cargo_args: list[str]) -> int:
    """Run cargo from the ast-grep source checkout, returning the exit code.

    Runs cargo via `RunCargo` (bundled toolchain) from `AST_GREP_SRC_DIR` so the
    checkout's `.cargo/config.toml` is honoured, with the installed
    cargo-subcommand `bin/` on `PATH` so external subcommands (cargo-auditable,
    cargo-audit) resolve. `RunCargo` has no cwd/env parameters, hence the manual
    save/restore (`contextlib.chdir` would be cleaner but is unknown to our
    pinned pylint).
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

    Delegates the cargo invocation to `run_cargo.RunCargo` (mirroring
    `tools/crates/run_gnrt.py`) so the bundled toolchain, and on Windows the
    hermetic VS toolchain flags, are applied. The build runs under
    `cargo auditable` so the binary embeds its dependency tree, letting
    `_audit_ast_grep` later scan exactly what was compiled in. All cargo state
    is pinned under the intermediate dir (target dir, cargo home).
    """
    AST_GREP_INTERMEDIATE_DIR.mkdir(parents=True, exist_ok=True)
    AST_GREP_DIR.mkdir(parents=True, exist_ok=True)

    # cargo-auditable embeds the dependency graph into the binary so the audit
    # scans only the crates actually compiled in (not unrelated workspace
    # members such as the Python/Node/wasm bindings).
    _install_cargo_tool('cargo-auditable')

    target_dir = AST_GREP_INTERMEDIATE_DIR / 'target'
    logging.info('Building ast-grep with the Chromium Rust toolchain (%s)',
                 DEFAULT_SYSROOT)
    returncode = _run_cargo_in_src([
        'auditable', 'build', '--locked', '--release', '--bin', 'ast-grep',
        '--target-dir',
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
    """Audit the dependencies compiled into the ast-grep binary.

    `cargo audit bin` reads the dependency tree embedded by `_build_ast_grep`'s
    `cargo auditable` build and checks exactly those crates against the RustSec
    advisory database -- so sibling workspace members we don't ship (the
    Python/Node/wasm bindings) are excluded. A non-zero exit -- a known
    vulnerability in a shipped dependency -- fails the build before the
    toolchain is packaged.
    """
    _install_cargo_tool('cargo-audit')
    logging.info('Auditing ast-grep binary dependencies with cargo audit')
    returncode = _run_cargo_in_src(['audit', 'bin', str(AST_GREP_BIN)])
    if returncode != 0:
        raise RuntimeError(f'cargo audit reported issues (exit {returncode})')


def _clean() -> None:
    """Remove the source clone and both toolchain directories."""
    for path in (AST_GREP_SRC_DIR, AST_GREP_INTERMEDIATE_DIR, AST_GREP_DIR):
        if path.exists():
            logging.info('Removing %s', path)
            shutil.rmtree(path)


def build(jobs: int, clean: bool = False) -> None:
    """Build `ast-grep` into `third_party/ast-grep/`, then audit its deps.

    Optionally removes the prior source/build directories first. Shared with
    `package_ast_grep.py` so packaging reuses the exact same build and audit.
    """
    if clean:
        _clean()
    _check_rust_toolchain()
    _clone_ast_grep()
    _build_ast_grep(jobs)
    _audit_ast_grep()


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

    build(args.jobs, clean=args.clean)

    logging.info('Done.')
    logging.info('ast-grep: %s', AST_GREP_BIN)
    return 0


if __name__ == '__main__':
    sys.exit(main())
