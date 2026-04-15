#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build and package a minimal Rust toolchain subset for Chromium.

Keep this as a *standalone* script, that can be used directly with a vanilla
Chromium checkout, with no additional dependencies.

To use this straight from Github just call:

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/rust/build_rust_toolchain_standalone.py \
    | python3 - \
        --out-dir=./out/ \
        --chromium-src=~/dev/chromium/src/
```

If you do not have a Chromium checkout yet, pass `--clone-chromium` together
with `--use-ref` to have the script fetch one automatically:

```sh
python3 build_rust_toolchain_standalone.py \
    --out-dir=./out/ \
    --chromium-src=~/dev/chromium/src/ \
    --clone-chromium \
    --use-ref=refs/heads/main
```

The output of this script is a .tar.xz archive containing two artifacts built
against the Chromium-managed LLVM/Clang installation:

  * rust-lld  — Rust's copy of the LLD linker, taken from the Chromium-built
                LLVM install tree (`RUST_HOST_LLVM_INSTALL_DIR/bin/lld`).
  * wasm32-unknown-unknown  — the stage-1 standard-library sysroot for the
                              bare-metal WebAssembly target, taken from the
                              Rust bootstrap build tree
                              (`RUST_BUILD_DIR/<triple>/stage1/lib/rustlib/`).

The build is driven by two scripts that live in `tools/rust/` inside the
Chromium source tree:

  * `build_rust.py`    — clones the Rust repository, builds LLVM/Clang via
                         `tools/clang/scripts/build.py`, generates
                         `config.toml` from `config.toml.template`, and
                         runs `x.py` (the Rust bootstrap driver) to compile
                         the toolchain.  The `--prepare-run-xpy` flag stops
                         after setup; `--run-xpy` then forwards extra
                         arguments directly to `x.py` with the correct
                         environment variables in place.

  * `package_rust.py`  — strips and packages the full Rust toolchain output
                           into a `.tar.xz` archive for upload to GCS.  This
                           script is only imported here for the
                           `RUST_TOOLCHAIN_PACKAGE_NAME` constant, which
                           provides the version-stamped base filename for the
                           output archive.

"""

import argparse
import contextlib
import importlib
import logging
from pathlib import Path, PurePath
import os
import platform
import shutil
import subprocess
import sys
import tarfile
from types import ModuleType

# Filename of the LLVM linker binary produced by the Chromium LLVM build.
LLD = 'lld' + ('.exe' if sys.platform == 'win32' else '')

# Name under which the LLD binary is stored inside the output archive.
RUST_LLD = f'rust-{LLD}'

# Rust target triple for the bare-metal WebAssembly target.  Included in the
# build so that Chromium can compile Rust code targeting WebAssembly.
WASM32_UNKNOWN_UNKNOWN = 'wasm32-unknown-unknown'

# Relative path (within tools/rust/) of the Rust bootstrap configuration
# template. build_rust.py generates config.toml from this file.
CONFIG_TOML_TEMPLATE = PurePath('config.toml.template')

# Relative path (within the Chromium src/ root) of the Rust toolchain scripts.
TOOLS_RUST = PurePath('tools') / 'rust'

# Relative path (within RUST_BUILD_DIR/<target_triple>/) to the stage-1
# rustlib output directory.  The wasm32 standard-library sysroot lives at
# <RUST_BUILD_DIR>/<triple>/stage1/lib/rustlib/wasm32-unknown-unknown/.
STAGE1_RUSTLIB = PurePath('stage1') / 'lib' / 'rustlib'

# The path for depot_tools under Chromium src.
DEPOT_TOOLS_REL_PATH = PurePath('third_party/depot_tools/')

# Relative path (within the Chromium src/ root) of the depot_tools vpython3
VPYTHON_PATH = DEPOT_TOOLS_REL_PATH / 'vpython3'

# Latest Chromium depot_tools bundle
DEPOT_TOOLS_URL = 'https://chromium.googlesource.com/chromium/tools/depot_tools'


def _check_call(*command, cwd=None):
    """Run *command* as a subprocess, logging the invocation and any stderr.

    Logs the full command string at INFO level before executing it.  If the
    process exits with a non-zero return code, any captured stderr is logged
    at WARNING level before the `CalledProcessError` is re-raised.

    Args:
        *command: The program and its arguments (passed as positional args,
            not as a list).
        cwd: Optional working directory for the subprocess.  Defaults to the
            caller's current working directory when `None`.

    Raises:
        subprocess.CalledProcessError: If the process exits with a non-zero
            return code.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))
    try:
        subprocess.run(command, cwd=cwd, check=True, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        if e.stderr:
            logging.warning(e.stderr.decode('utf-8', errors='replace').strip())
        raise


class ToolchainBuilder:
    """Orchestrate a minimal Rust toolchain build and package it as a .tar.xz.

    The build process has three phases:

    1. **Prepare** (`_prepare_run_xpy`): Runs `build_rust.py
       --prepare-run-xpy`.  This performs all one-time setup — cloning the
       Rust source tree, building LLVM/Clang, and generating `config.toml`
       from `config.toml.template` — but stops before invoking `x.py`.

    2. **Build** (`_run_xpy`): Runs `build_rust.py --run-xpy -- build`
       with `--stage 1` targeting both the host triple and
       `wasm32-unknown-unknown`.  This compiles the stage-1 Rust compiler
       and standard library using the previously generated configuration.

    3. **Package** (`_create_archive`): Assembles the output `.tar.xz`
       archive from two sources inside the build tree:

       * The `lld` binary from `RUST_HOST_LLVM_INSTALL_DIR/bin/`,
         stored as `rust-lld` in the archive.
       * The `wasm32-unknown-unknown` standard-library sysroot directory
         from the stage-1 rustlib output, stored verbatim.

    Phases 1 and 2 are wrapped in `_temporary_config_toml_template_edits`,
    which appends `profiler = false` for the wasm32 target to
    `config.toml.template` and restores the file (via `git checkout`)
    both before the build starts and in a `finally` block afterwards.
    """

    def __init__(self,
                 chromium_src: str,
                 out_dir: str,
                 clone_chromium: bool = False):
        """Validate inputs, resolve paths, and import the build/package modules.

        Args:
            chromium_src: Path to the Chromium `src/` directory.  Must exist
                and contain a `tools/rust/` subdirectory.
            out_dir: Directory where the output `.tar.xz` archive is written.
                Created (including parents) if it does not already exist.
            clone_chromium: Whether the builder should operate in a mode that
                allows cloning Chromium automatically, if necessary.

        Raises:
            RuntimeError: If `chromium_src` does not exist or does not look
                like a Chromium src tree, or if `out_dir` exists but is not a
                directory.
        """
        self.chromium_src: Path = Path(chromium_src).expanduser().resolve()
        if not self._has_valid_chromium_path():
            if clone_chromium:
                logging.info('Chromium src not found at %s, cloning...',
                             self.chromium_src)
                self._clone_chromium()
            else:
                raise RuntimeError(
                    '--chromium-src must be an existing Chromium src '
                    f'directory: {chromium_src}')

        # Absolute path to tools/rust/ inside the Chromium source tree.
        self.tools_rust: Path = self.chromium_src / TOOLS_RUST
        if not self.tools_rust.is_dir():
            raise RuntimeError(f'{self.tools_rust} is not a directory.')

        # Absolute path to depot_tools vpython3 inside the Chromium checkout.
        self.vpython_path: Path = self.chromium_src / VPYTHON_PATH

        # Resolved absolute path to the output directory.
        self.out_dir: Path = Path(out_dir).expanduser().resolve()
        if self.out_dir.exists() and not self.out_dir.is_dir():
            raise RuntimeError(
                f'--out-dir must be a directory path: {out_dir}')
        if not self.out_dir.exists():
            self.out_dir.mkdir(parents=True)

        # Absolute path to tools/rust/config.toml.template.  This file is
        # temporarily edited during the build to add wasm32 profiler settings.
        self.config_toml_template: Path = self.tools_rust / CONFIG_TOML_TEMPLATE

        tools_rust_str: str = str(self.tools_rust)
        if tools_rust_str not in sys.path:
            sys.path.insert(0, tools_rust_str)
        # Dynamically imported tools/rust/build_rust.py module.
        self.build_rust_module: ModuleType = importlib.import_module(
            'build_rust')
        # Dynamically imported tools/rust/package_rust.py module.
        self.package_rust_module: ModuleType = importlib.import_module(
            'package_rust')

    @contextlib.contextmanager
    def _temporary_config_toml_template_edits(self):
        """Context manager: patch `config.toml.template` for the build.

        `build_rust.py` generates `config.toml` from
        `tools/rust/config.toml.template`.  For the wasm32 target we need
        to set `profiler = false` (otherwise the profiling infrastructure
        required by the full toolchain build is pulled in, but is not needed
        for our subset).

        Protocol:
        1. Restore the template to its HEAD state via `git checkout` before
           any edits — guards against a dirty file left by a previous
           interrupted run.
        2. Append the `[target.wasm32-unknown-unknown]` stanza.
        3. `yield` so the caller can run the build.
        4. Unconditionally restore the template in a `finally` block.
        """

        def _restore_config_toml_template():
            _check_call('git', '-C', str(self.chromium_src), 'checkout', '--',
                        str(self.config_toml_template))

        _restore_config_toml_template()
        with self.config_toml_template.open('a') as file:
            file.write(
                f'\n[target.{WASM32_UNKNOWN_UNKNOWN}]\nprofiler = false\n')

        try:
            yield
        finally:
            _restore_config_toml_template()

    def _prepare_run_xpy(self):
        """Set up the Rust checkout so that x.py can be run subsequently.

        Invokes `build_rust.py --prepare-run-xpy`, which performs all
        one-time preparation steps:

        * Clones / updates the Rust source tree to the pinned revision.
        * Builds LLVM and Clang via `tools/clang/scripts/build.py`.
        * Generates `config.toml` from the (already-patched)
          `config.toml.template`.

        After this call returns, the build directory is ready for
        `_run_xpy` to invoke x.py directly without repeating the setup.
        """
        _check_call(str(self.vpython_path),
                    'build_rust.py',
                    '--prepare-run-xpy',
                    cwd=self.tools_rust)

    def _run_xpy(self):
        """Compile the stage-1 Rust toolchain via x.py.

        Invokes `build_rust.py --run-xpy -- build` with the following flags:

        * `--build <host-triple>`  — the native host target (e.g.
          `x86_64-unknown-linux-gnu`) as returned by
          `build_rust.RustTargetTriple()`.
        * `--target <host-triple>,wasm32-unknown-unknown`  — build both the
          host and the bare-metal WebAssembly standard library.
        * `--stage 1`  — stop after the stage-1 compiler and stdlib; a full
          stage-2 build is not required for our purposes.

        The resulting artifacts are placed under
        `RUST_BUILD_DIR/<host-triple>/stage1/` by the Rust bootstrap.
        """
        target_triple: str = self.build_rust_module.RustTargetTriple()

        _check_call(str(self.vpython_path),
                    'build_rust.py',
                    '--run-xpy',
                    '--',
                    'build',
                    '--build',
                    target_triple,
                    '--target',
                    f'{target_triple},{WASM32_UNKNOWN_UNKNOWN}',
                    '--stage',
                    '1',
                    cwd=self.tools_rust)

    def _package_name(self) -> str:
        """Return the filename for the output archive.

        The name is composed of a platform prefix followed by
        `package_rust.RUST_TOOLCHAIN_PACKAGE_NAME` (a version-stamped string
        of the form `rust-toolchain-<clang+rust-revision>.tar.xz`).

        Platform prefixes mirror the GCS convention used by `package_rust.py`
        and `tools/clang/scripts/upload.sh`:

        +------------------+-------------------+
        | Platform         | Prefix            |
        +==================+===================+
        | macOS (ARM)      | `mac-arm64`       |
        +------------------+-------------------+
        | macOS (Intel)    | `mac`             |
        +------------------+-------------------+
        | Windows          | `win`             |
        +------------------+-------------------+
        | Linux / other    | `linux-x64`       |
        +------------------+-------------------+
        """
        if sys.platform == 'darwin':
            if platform.machine() == 'arm64':
                platform_prefix = 'mac-arm64'
            else:
                platform_prefix = 'mac'
        elif sys.platform == 'win32':
            platform_prefix = 'win'
        else:
            platform_prefix = 'linux-x64'

        return (f'{platform_prefix}-'
                f'{self.package_rust_module.RUST_TOOLCHAIN_PACKAGE_NAME}')

    def _create_archive(self):
        """Write the output .tar.xz archive to `self.out_dir`.

        The archive (named via `_package_name()`) contains exactly two
        entries:

        * `rust-lld` — the LLD linker binary from
          `RUST_HOST_LLVM_INSTALL_DIR/bin/lld[.exe]`.  Rust's toolchain
          ships its own copy of LLD under this name so that `rustc` can
          link without requiring a system linker.
        * `wasm32-unknown-unknown/` — the stage-1 standard-library sysroot
          directory located at
          `RUST_BUILD_DIR/<host-triple>/stage1/lib/rustlib/wasm32-unknown-unknown/`.
          This directory contains the precompiled `core`, `alloc`, and
          `std` libraries needed to compile Rust code for the bare-metal
          WebAssembly target.
        """
        target_triple = self.build_rust_module.RustTargetTriple()
        stage1_output_path = (Path(self.build_rust_module.RUST_BUILD_DIR) /
                              target_triple / STAGE1_RUSTLIB)
        output_archive = self.out_dir / self._package_name()

        with tarfile.open(output_archive, 'w:xz') as tar:
            tar.add(Path(self.build_rust_module.RUST_HOST_LLVM_INSTALL_DIR) /
                    'bin' / LLD,
                    arcname=RUST_LLD)
            tar.add(stage1_output_path / WASM32_UNKNOWN_UNKNOWN,
                    arcname=WASM32_UNKNOWN_UNKNOWN)

    def _bootstrap_depot_tools(self):
        """Clone depot_tools into this builder's Chromium checkout."""
        if shutil.which('gclient') is not None:
            logging.debug('depot_tools already on PATH, skipping clone')
            return

        depot_tools_path = self.chromium_src / DEPOT_TOOLS_REL_PATH
        logging.info('Installing depot_tools under %s', depot_tools_path)
        depot_tools_path.parent.mkdir(parents=True, exist_ok=True)
        _check_call('git', 'clone', DEPOT_TOOLS_URL, str(depot_tools_path))

        # Add depot_tools to PATH so that gclient can be used.
        os.environ['PATH'] = os.pathsep.join(
            [str(depot_tools_path), os.environ['PATH']])
        _check_call('gclient')

    def _has_valid_chromium_path(self) -> bool:
        """Return whether self.chromium_src points to a valid Chromium repo."""
        if not self.chromium_src.exists():
            return False

        if not self.chromium_src.is_dir():
            raise RuntimeError(
                f'--chromium-src exists but is not a directory: '
                f'{self.chromium_src}')

        logging.info('Checking for valid Chromium repo at %s',
                     self.chromium_src)
        try:
            _check_call('git',
                        'log',
                        '-1',
                        '--oneline',
                        'chrome/VERSION',
                        cwd=self.chromium_src)
        except (subprocess.CalledProcessError, OSError):
            return False

        return True

    def _checkout_chromium_ref(self, ref: str):
        """Check out a specific Chromium ref and resync dependencies."""
        logging.info('Checking out Chromium ref %s', ref)
        try:
            _check_call('git',
                        'checkout',
                        '--force',
                        ref,
                        cwd=self.chromium_src)
        except subprocess.CalledProcessError:
            logging.info('Ref %s not found locally, fetching from origin', ref)
            _check_call('git', 'fetch', 'origin', ref, cwd=self.chromium_src)
            _check_call('git',
                        'checkout',
                        '--force',
                        'FETCH_HEAD',
                        cwd=self.chromium_src)

        _check_call('gclient', 'sync', '--force', '-D', cwd=self.chromium_src)
        _check_call('git',
                    'log',
                    '-1',
                    '--oneline',
                    'chrome/VERSION',
                    cwd=self.chromium_src)

    def _clone_chromium(self):
        """Clone a fresh Chromium checkout under `self.chromium_src.parent`."""
        self._bootstrap_depot_tools()

        self.chromium_src.parent.mkdir(parents=True, exist_ok=True)
        _check_call('fetch',
                    '--nohistory',
                    '--nohooks',
                    'chromium',
                    cwd=self.chromium_src.parent)

    def run(self, use_ref: str = None):
        """Execute the full build-and-package pipeline.

        Coordinates the three phases in order:

        1. Within `_temporary_config_toml_template_edits`:
           a. `_prepare_run_xpy` — clone, build LLVM, generate config.toml.
           b. `_run_xpy` — compile stage-1 Rust + wasm32 stdlib via x.py.
        2. `_create_archive` — assemble the output .tar.xz.

        `config.toml.template` is returned to its original state always.

        Args:
            use_ref: Optional Git reference (branch, tag, or commit) to check
                out before building. If provided, calls `_checkout_chromium_ref`
                first.
        """
        if use_ref:
            self._checkout_chromium_ref(use_ref)

        # Build process
        with self._temporary_config_toml_template_edits():
            self._prepare_run_xpy()
            self._run_xpy()

        self._create_archive()


def main():
    """Parse CLI arguments, configure logging, and run the toolchain builder."""
    parser = argparse.ArgumentParser(
        description='Build and package rust-lld and wasm32-unknown-unknown')
    parser.add_argument('--chromium-src',
                        required=True,
                        help='Path to Chromium src/ directory')
    parser.add_argument('--out-dir',
                        required=True,
                        help='Output directory for the archive')
    parser.add_argument('--clone-chromium',
                        action='store_true',
                        help='Allow cloning Chromium if needed')
    parser.add_argument(
        '--use-ref',
        help='Git reference (branch, tag, commit) to check out before building'
        ' the toolchain.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging')
    args = parser.parse_args()

    if args.clone_chromium and not args.use_ref:
        parser.error('--use-ref is required when --clone-chromium is provided')

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    builder = ToolchainBuilder(args.chromium_src, args.out_dir,
                               args.clone_chromium)
    builder.run(args.use_ref)
    return 0


if __name__ == '__main__':
    sys.exit(main())
