#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# [VPYTHON:BEGIN]
# python_version: "3.11"
#
# wheel: <
#   name: "infra/python/wheels/pyyaml-py3"
#   version: "version:6.0.1"
# >
# [VPYTHON:END]
"""Build and package a minimal Rust toolchain subset for Chromium.

By default the builder produces a minimal overlay `.tar.xz` containing only two
artifacts built against the Chromium-managed LLVM/Clang installation, sitting on
top of the prebuilt Rust toolchain that gclient already syncs.

With `--full-toolchain`, the script instead packages the *complete* Rust
toolchain, exactly as Chromium's `tools/rust/package_rust.py` does, and then
overlays the bare-metal WebAssembly standard-library sysroot onto that archive.
That mode is provided to work around upstream issues that cause reproducibility
problems relating to cherry-picks and the fact that the `rustc` compiler encodes
the value of `HEAD`.

In both modes members are stored at paths relative to a Rust toolchain root, so
the archive can be extracted directly over `src/third_party/rust-toolchain`:

  * bin/rust-lld  — Rust's copy of the LLD linker, taken from the Chromium-built
                    LLVM install tree (`RUST_HOST_LLVM_INSTALL_DIR/bin/lld`).
                    `package_rust.py` does not install this, so it is added in
                    both modes.
  * lib/rustlib/wasm32-unknown-unknown  — the stage-1 standard-library sysroot
                              for the bare-metal WebAssembly target, taken from
                              the Rust bootstrap build tree
                              (`RUST_BUILD_DIR/<triple>/stage1/lib/rustlib/`).
                              Added in both modes.

The output archive is named:

    <platform>-rust-toolchain-<RUST_REVISION>-<RUST_SUB_REVISION>
        -llvmorg-<CLANG_REVISION>-<BRAVE_SUB_REVISION>.tar.xz

For example, on Linux with `--brave-subrevision=1`:

    linux-x64-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-2
        -llvmorg-23-init-10931-g20b6ec66-1.tar.xz

`<BRAVE_SUB_REVISION>` is supplied by the caller via `--brave-subrevision`. Bump
it for every Brave-side respin that should land as a distinct sibling archive,
including rebuilds against a different Chromium version with the same upstream
Rust+Clang stack, since the Chromium version is no longer encoded in the
filename.

Alongside the archive, a sibling YAML index sharing its name stem is also
written to `--out-dir`:

    <platform>-rust-toolchain-<RUST_REVISION>-<RUST_SUB_REVISION>
        -llvmorg-<CLANG_REVISION>-<BRAVE_SUB_REVISION>.yaml

The index records the just-built tarball's metadata (bucket URL, SHA-256, size,
Chromium/brave-core provenance). Publishing refuses if an index already exists
for this exact `<BRAVE_SUB_REVISION>` (bump `--brave-subrevision` for a fresh
respin instead). With `--upload`, the archive and its sibling index are
published to `TOOLCHAIN_BUCKET_URL`.

The build is driven by two scripts that live in `tools/rust/` inside the
Chromium source tree:

  * `build_rust.py`    — clones the Rust repository, builds LLVM/Clang via
                        `tools/clang/scripts/build.py`, generates `config.toml`
                        from `config.toml.template`, and runs `x.py` (the Rust
                        bootstrap driver) to compile the toolchain.  The
                        `--prepare-run-xpy` flag stops after setup; `--run-xpy`
                        then forwards extra arguments directly to `x.py`with the
                        correct environment variables in place.

  * `package_rust.py`  — strips and packages the full Rust toolchain output
                           into a `.tar.xz` archive for upload to GCS.  This
                           script is only imported here for the
                           `RUST_TOOLCHAIN_PACKAGE_NAME` constant, which
                           provides the version-stamped base filename for the
                           output archive.

"""

from __future__ import annotations

import argparse
import contextlib
from datetime import datetime, timezone
import importlib
import logging
import lzma
from pathlib import Path
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import tempfile
import tomllib
from types import ModuleType

# This is necessary because these scripts are used by brockit too.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from cherry_picks import (  # pylint: disable=wrong-import-position
    _check_call, cherry_picks)
import toolchain_publish  # pylint: disable=wrong-import-position
from upload import sha256_file  # pylint: disable=wrong-import-position

# Executable suffix for host tools on the current platform.
EXE_SUFFIX = '.exe' if sys.platform == 'win32' else ''

# Filename of the LLVM linker binary produced by the Chromium LLVM build.
LLD = 'lld' + EXE_SUFFIX

# Basenames of the prebuilt host tools used as bootstrap's stage-1 compiler in
# `--no-full-toolchain` mode (see `--use-prebuilt-rustc`). They live under
# `RUST_TOOLCHAIN_OUT_DIR/bin/` and carry the platform's executable suffix.
RUSTC = f'rustc{EXE_SUFFIX}'
CARGO = f'cargo{EXE_SUFFIX}'

# Basename under which the LLD binary is stored inside the output archive (its
# full archive path is RUST_LLD_ARCNAME).
RUST_LLD = f'rust-{LLD}'

# Filename of the standalone MSVC-style librarian produced by the LLVM build.
# Shipped (Windows only) so that `cc-rs`-driven cargo invocations have an AR
# they can call directly without flag-injection workarounds.  Chromium's
# distributed clang package strips this (see tools/clang/scripts/package.py),
# but RUST_HOST_LLVM_INSTALL_DIR has it.  Mirrors what upstream
# tools/rust/config.toml.template references via `$LLVM_BIN/llvm-lib.exe`.
LLVM_LIB = 'llvm-lib.exe'

# Rust target triple for the bare-metal WebAssembly target.  Included in the
# build so that Chromium can compile Rust code targeting WebAssembly.
WASM32_UNKNOWN_UNKNOWN = 'wasm32-unknown-unknown'

# Member paths for each artifact inside the output archive, laid out relative
# to a Rust toolchain root (i.e. `src/third_party/rust-toolchain`).  The archive
# mirrors the final on-disk layout so a consumer can extract it straight over
# the toolchain directory without knowing anything about its contents — no
# per-file moves and no platform-specific name handling on the client side.
# Forward slashes are used deliberately: they are the portable tar separator on
# every platform.
RUST_LLD_ARCNAME = f'bin/{RUST_LLD}'
WASM32_ARCNAME = f'lib/rustlib/{WASM32_UNKNOWN_UNKNOWN}'
LLVM_LIB_ARCNAME = f'bin/{LLVM_LIB}'

# Minimal wasm32 sources used by `_smoke_test_wasm` to exercise the packaged
# toolchain. The rlib variant is compile-only; the cdylib variant additionally
# drives the packaged rust-lld over the wasm objects.
WASM_SMOKE_RLIB_SRC = """\
pub fn smoke() -> String {
    let v = vec![1u32, 2, 3];
    format!("{}", v.iter().sum::<u32>())
}
"""

WASM_SMOKE_CDYLIB_SRC = """\
#[no_mangle]
pub extern "C" fn smoke() -> u32 {
    let v = vec![1u32, 2, 3];
    v.iter().sum()
}
"""

# Relative path (within tools/rust/) of the Rust bootstrap configuration
# template. build_rust.py generates config.toml from this file.
CONFIG_TOML_TEMPLATE = Path('config.toml.template')

# Relative path (within the Chromium src/ root) of the Rust toolchain scripts.
TOOLS_RUST = Path('tools') / 'rust'

# Relative path (within RUST_BUILD_DIR/<host-triple>/) to the from-scratch
# stage-1 rustlib output.  The default (full-toolchain) build's wasm32 sysroot
# is assembled by x.py at
# <RUST_BUILD_DIR>/<host>/stage1/lib/rustlib/wasm32-unknown-unknown/.
STAGE1_RUSTLIB = Path('stage1') / 'lib' / 'rustlib'

# Relative paths (within RUST_BUILD_DIR/<host-triple>/) to the prebuilt-compiler
# stage-0 build output.
STAGE0_STD = Path('stage0-std')
STAGE0_RUSTLIB = Path('stage0-sysroot') / 'lib' / 'rustlib'

# vpython3 that is selected by `depot_tools` from `$PATH`.
# This little Windows specific quirk is only needed when calling this script on
# Windows using git bash.
VPYTHON_PATH = Path('third_party/depot_tools') / (
    'vpython3.bat' if sys.platform == 'win32' else 'vpython3')

# This source is used as a token to check if we have a valid Chromium repo as
# it is one of those reliable files that are always present in any version.
CHROME_VERSION_FILE = Path('chrome/VERSION')

# Upstream cherry-picks applied onto the checkout for the build's duration.
# Empty for now.
CLANG_CHERRY_PICK_COMMITS = []

# The bucket + key prefix in our infra where the rust toolchain is archived.
TOOLCHAIN_BUCKET = 'brave-build-deps-public'
TOOLCHAIN_BUCKET_PREFIX = 'rust-toolchain-aux'
TOOLCHAIN_BUCKET_URL = (
    f'https://{TOOLCHAIN_BUCKET}.s3.brave.com/{TOOLCHAIN_BUCKET_PREFIX}')

# Every platform a Rust/WASM toolchain is published for, mapped to the gclient
# host condition `install_extra_deps.py` uses to select the matching
# archive. Keyed by the prefix `_platform_prefix` emits. Single source of truth
# for "what platforms exist", consumed by `rust_toolchain_extra_dep`.
SUPPORTED_PLATFORM_CONDITIONS = {
    'linux-x64': 'host_os == "linux"',
    'mac-arm64': 'host_os == "mac" and host_cpu == "arm64"',
    'mac': 'host_os == "mac" and host_cpu == "x64"',
    'win': 'host_os == "win"',
}

# Chromium GCS `host_os` directory for each supported platform prefix. The Brave
# WASM archive is an *overlay* on top of the upstream Chromium-built Rust
# toolchain that gclient downloads as a `gcs` dep, so each published object
# records the upstream archive it sits on top of (`overlayed_on`). This maps our
# prefix to the directory that archive lives under in the clang bucket,
# mirroring the `host_os` list in
# `tools/clang/scripts/sync_deps.py:GetRustObjectNames`.
PLATFORM_PREFIX_TO_CHROMIUM_HOST_OS = {
    'linux-x64': 'Linux_x64',
    'mac-arm64': 'Mac_arm64',
    'mac': 'Mac',
    'win': 'Win',
}

# Install path + dep-level condition of the published Rust/WASM toolchain in
# `install_extra_deps.py`'s `EXTRA_DEPS`. `rust_toolchain_extra_dep`
# returns the entry keyed by this path.
RUST_TOOLCHAIN_DEP_PATH = 'src/third_party/rust-toolchain'
RUST_TOOLCHAIN_DEP_CONDITION = 'not rust_force_head_revision'

if sys.platform == 'win32':
    # Path to Git's sh.exe on Windows, which is used by
    # `tools/rust/build_rust.py` to build the toolchain on Windows.`
    GIT_SH_PRESUMED_BIN_PATH = Path(r'C:\Program Files\Git\bin\sh.exe')


def toolchain_index_name(platform_prefix: str, upstream_stem: str,
                         brave_subrevision: int) -> str:
    """`<platform>-<upstream_stem>-<brave_subrevision>.yaml`.

    The sibling YAML index for one platform's archive, sharing its name stem.
    """
    return f'{platform_prefix}-{upstream_stem}-{brave_subrevision}.yaml'


def _fetch_index_object(index_url: str, expected_object_name: str) -> dict:
    """Download one platform's sibling YAML index and return its entry.

    Returns the mapping `ToolchainBuilder._write_index` published (`url`,
    `sha256sum`, `size_bytes`, ...). `expected_object_name` is the archive
    basename the index must point at -- a mismatch means the index was
    fetched from the wrong key, and is treated as a hard error rather than
    silently trusted.

    Raises:
        RuntimeError: if the index cannot be fetched or is malformed.
    """
    entry = toolchain_publish.fetch_index(index_url, 'rust toolchain')
    if (not isinstance(entry, dict) or 'url' not in entry
            or 'sha256sum' not in entry or 'size_bytes' not in entry):
        raise RuntimeError(
            f'Malformed toolchain index {index_url}: expected a mapping with '
            f'"url", "sha256sum", and "size_bytes", got {entry!r}')
    object_name = entry['url'].rsplit('/', 1)[-1]
    if object_name != expected_object_name:
        raise RuntimeError(
            f'Toolchain index {index_url} yielded {object_name!r}, expected '
            f'{expected_object_name!r}.')
    return entry


def rust_toolchain_extra_dep(upstream_stem: str,
                             brave_subrevision: int) -> dict[str, dict]:
    """Assemble the complete `EXTRA_DEPS` entry for the published toolchain.

    This function composes a Python dictionary representing the full
    `EXTRA_DEPS` value that should be used in `install_extra_deps.py` for
    the current Chromium checkout.

        {
          'src/third_party/rust-toolchain': {
            'bucket': '<bucket>/',
            'condition': 'not rust_force_head_revision',
            'objects': [
              {'object_name': ..., 'sha256sum': ..., 'size_bytes': ...,
               'overlayed_on': ..., 'condition': ...},
              ...
            ],
          },
        }

    This function is used by brockit to patch the value of `EXTRA_DEPS` in
    `install_extra_deps.py`.

    Args:
        upstream_stem: The upstream toolchain package stem (i.e.
            `package_rust.RUST_TOOLCHAIN_PACKAGE_NAME` without the `.tar.xz`
            suffix).
        brave_subrevision: The exact Brave respin counter to pin, naming each
            platform's sibling index (see `toolchain_index_name`).

    Raises:
        RuntimeError: if any platform's index is missing or malformed.
    """
    objects = []
    for platform_prefix, condition in SUPPORTED_PLATFORM_CONDITIONS.items():
        stem = f'{platform_prefix}-{upstream_stem}'
        object_name = f'{stem}-{brave_subrevision}.tar.xz'
        index_name = toolchain_index_name(platform_prefix, upstream_stem,
                                          brave_subrevision)
        index_url = f'{TOOLCHAIN_BUCKET_URL}/{index_name}'
        entry = _fetch_index_object(index_url, object_name)
        host_os = PLATFORM_PREFIX_TO_CHROMIUM_HOST_OS[platform_prefix]
        objects.append({
            'object_name': object_name,
            'sha256sum': entry['sha256sum'],
            'size_bytes': entry['size_bytes'],
            # Upstream Chromium-built Rust toolchain this archive overlays; see
            # `sync_deps.GetRustObjectNames` for the matching naming scheme.
            'overlayed_on': f'{host_os}/{upstream_stem}.tar.xz',
            'condition': condition,
        })

    return {
        RUST_TOOLCHAIN_DEP_PATH: {
            'bucket': f'{TOOLCHAIN_BUCKET_URL}/',
            'condition': RUST_TOOLCHAIN_DEP_CONDITION,
            'objects': objects,
        }
    }


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
         stored as `bin/rust-lld` in the archive.
       * The `wasm32-unknown-unknown` standard-library sysroot directory
         from the stage-1 rustlib output, stored under `lib/rustlib/`.

    Phases 1 and 2 are wrapped in `_temporary_config_toml_template_edits`,
    which appends a `[target.wasm32-unknown-unknown]` stanza to
    `config.toml.template` (inherited from the host stanza).
    """

    def __init__(self, chromium_src: str, out_dir: str,
                 brave_subrevision: int):
        """ Initialses the builder fields.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
            out_dir: Directory where the output `.tar.xz` archive is written.
            brave_subrevision: Integer respin counter encoded as the last
                section of the output archive name.  See
                `_package_name` for the full naming schema.
        """
        # The absolute path to the Chromium source directory.
        self._chromium_src: Path = Path(chromium_src).expanduser().resolve()

        # path to tools/rust/ inside the Chromium source tree.
        self._tools_rust: Path = self._chromium_src / TOOLS_RUST

        # Absolute path to depot_tools vpython3 inside the Chromium checkout.
        self._vpython_path: Path = self._chromium_src / VPYTHON_PATH

        # path to the output directory where the toolchain will be written.
        self._out_dir: Path = Path(out_dir).expanduser().resolve()

        # Absolute path to tools/rust/config.toml.template.  This file is
        # temporarily edited during the build to add wasm32 profiler settings.
        self._config_toml_template: Path = (self._tools_rust /
                                            CONFIG_TOML_TEMPLATE)

        # Module for tools/rust/build_rust.py. Initialised by `run()`.
        self._build_rust_module: ModuleType | None = None

        # Module for tools/rust/package_rust.py. Initialised by `run()`.
        self._package_rust_module: ModuleType | None = None

        # Integer respin counter encoded as the last section of the
        # output archive name.
        self._brave_subrevision: int = brave_subrevision

    def _native_target_stanza(self) -> dict[str, str | bool]:
        """Return the `[target.<native-triple>]` table from the template.

        `$LLVM_BIN` placeholders inside string values are preserved verbatim
        — `build_rust.py` substitutes them when it generates `config.toml`.
        Bare `$PLACEHOLDER` lines (not valid TOML) are stripped before
        parsing.
        """
        target_triple = self._build_rust_module.RustTargetTriple()
        text = self._config_toml_template.read_bytes().decode('utf-8')
        text = re.sub(r'(?m)^\$[A-Z_]+\s*$\n?', '', text)
        data = tomllib.loads(text)
        return dict(data['target'][target_triple])

    @staticmethod
    def _emit_toml_kv(key: str, value: str | bool) -> str:
        """Render a single key/value pair as a TOML assignment line."""
        if isinstance(value, bool):
            return f'{key} = {"true" if value else "false"}'
        return f'{key} = "{value}"'

    @contextlib.contextmanager
    def _temporary_config_toml_template_edits(self):
        """Context manager: patch `config.toml.template` for the build.

        `build_rust.py` generates `config.toml` from
        `tools/rust/config.toml.template`.  We append a
        `[target.wasm32-unknown-unknown]` stanza that inherits most of the
        host target's settings, and then we do a few changes to them.

        Protocol:
        1. Restore the template to its HEAD state via `git checkout` before
           any edits — guards against a dirty file left by a previous
           interrupted run.
        2. Append the `[target.wasm32-unknown-unknown]` stanza.
        3. `yield` so the caller can run the build.
        4. Unconditionally restore the template in a `finally` block.
        """

        def _restore_config_toml_template():
            _check_call('git', '-C', str(self._chromium_src), 'checkout', '--',
                        str(self._config_toml_template))

        _restore_config_toml_template()

        # Always filtering out linker, as WASM builds with rust-lld.
        wasm = {
            k: v
            for k, v in self._native_target_stanza().items()
            if k not in ('linker', 'jemalloc')
        }

        # The Windows host stanza names MSVC-style frontends from the LLVM
        # install (`clang-cl.exe`, `llvm-lib.exe`); wasm32's compiler-builtins
        # build expects GNU-style ones (`clang.exe`, `llvm-ar.exe`) which sit
        # in the same `bin/`. The replacements are no-ops on macOS/Linux
        # because the host stanzas there already use the GNU names.
        msvc_to_gnu = {
            'clang-cl.exe': 'clang.exe',
            'llvm-lib.exe': 'llvm-ar.exe',
        }

        def _swap(value: str) -> str:
            for msvc, gnu in msvc_to_gnu.items():
                value = value.replace(msvc, gnu)
            return value

        wasm = {
            k: _swap(v) if isinstance(v, str) else v
            for k, v in wasm.items()
        }

        # Disabling profiler for all configurations.
        wasm['profiler'] = False

        stanza = '\n'.join([
            f'[target.{WASM32_UNKNOWN_UNKNOWN}]',
            *(self._emit_toml_kv(k, v) for k, v in wasm.items())
        ])

        logging.info('Appending to %s:\n%s', self._config_toml_template,
                     stanza)
        with self._config_toml_template.open('a') as file:
            file.write('\n' + stanza + '\n')

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
        _check_call(str(self._vpython_path),
                    'build_rust.py',
                    '--prepare-run-xpy',
                    cwd=self._tools_rust)

    def _run_xpy(self, use_prebuilt_rustc: bool = False):
        """Compile the wasm32 standard library via x.py.

        Drives upstream `x.py` through `build_rust.py --run-xpy` in one of two
        modes:

        * Default (`use_prebuilt_rustc=False`): `build --stage 1 --target
          <host>,wasm32` builds the stage-1 compiler plus the host and wasm32
          standard libraries.  Artifacts land under
          `RUST_BUILD_DIR/<host>/stage1/`.

        * `use_prebuilt_rustc=True`: `build library --stage 0 --target wasm32`
          with `--set build.rustc`/`build.cargo` pointed at the prebuilt
          toolchain in `RUST_TOOLCHAIN_OUT_DIR` and `--set
          build.local-rebuild=true`.  `local-rebuild` tells bootstrap the
          stage-0 compiler is the same version as the in-tree sources, which is
          true here because `update_rust.py` pins the synced toolchain and
          `rust-src` to the same revision; without it bootstrap refuses to build
          at stage 0.  The synced compiler compiles the wasm32 std directly — no
          `rustc` is built — so the rlibs are byte-compatible with the toolchain
          it overlays.

          A stage-0 build does not assemble a `lib/rustlib/<target>/lib/`
          sysroot (the crates land in cargo's output dir), so
          `_assemble_stage0_wasm_sysroot` stitches one together afterwards.  A
          `--stage 1` build *would* assemble the sysroot, but it compiles a
          fresh stage-1 `rustc` and tags the std with it — which a consumer
          using the synced `rustc` then rejects with E0514.
        """
        target_triple: str = self._build_rust_module.RustTargetTriple()

        if use_prebuilt_rustc:
            prebuilt_bin = (
                Path(self._build_rust_module.RUST_TOOLCHAIN_OUT_DIR) / 'bin')
            rustc = prebuilt_bin / RUSTC
            cargo = prebuilt_bin / CARGO
            for tool in (rustc, cargo):
                if not tool.is_file():
                    raise RuntimeError(
                        f'Prebuilt Rust tool not found: {tool}. The '
                        f'--no-full-toolchain build uses the toolchain gclient '
                        f'syncs to {prebuilt_bin.parent} as bootstrap\'s '
                        f'stage-0; run `gclient sync` or pass --full-toolchain.'
                    )
            _check_call(str(self._vpython_path),
                        'build_rust.py',
                        '--run-xpy',
                        '--',
                        'build',
                        'library',
                        '--build',
                        target_triple,
                        '--target',
                        WASM32_UNKNOWN_UNKNOWN,
                        '--stage',
                        '0',
                        '--set',
                        f'build.rustc={rustc.as_posix()}',
                        '--set',
                        f'build.cargo={cargo.as_posix()}',
                        '--set',
                        'build.local-rebuild=true',
                        cwd=self._tools_rust)
            return

        _check_call(str(self._vpython_path),
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
                    cwd=self._tools_rust)

    def _stage1_wasm_stdlib_dir(self) -> Path:
        """Return the stage-1 wasm32 sysroot x.py assembles in the build tree.

        Used by the default (full-toolchain) build, where `_run_xpy` runs a
        from-scratch `--stage 1` build that assembles the sysroot under
        `stage1/` — see `STAGE1_RUSTLIB`.
        """
        target_triple = self._build_rust_module.RustTargetTriple()
        return (Path(self._build_rust_module.RUST_BUILD_DIR) / target_triple /
                STAGE1_RUSTLIB / WASM32_UNKNOWN_UNKNOWN)

    def _assemble_stage0_wasm_sysroot(self) -> Path:
        """Assemble a wasm32 sysroot from the prebuilt stage-0 build output.
        Raises:
            RuntimeError: if the stage-0 std output cannot be found.
        """
        target_triple = self._build_rust_module.RustTargetTriple()
        build_dir = Path(
            self._build_rust_module.RUST_BUILD_DIR) / target_triple

        std_out = build_dir / STAGE0_STD / WASM32_UNKNOWN_UNKNOWN
        profile_dir = next(
            (d for d in sorted(std_out.glob('*')) if any(d.glob('*.rlib'))),
            None)
        if profile_dir is None:
            raise RuntimeError(
                f'No stage-0 wasm32 std crates found under {std_out}; did the '
                f'`build library --stage 0` step run?')
        stamp = profile_dir / '.libstd-stamp'
        if not stamp.is_file():
            raise RuntimeError(
                f'No stage-0 wasm32 std stamp file found at {stamp}; did the '
                f'`build library --stage 0` step run?')

        # Assemble into a clean sysroot so stale crates from a prior run cannot
        # leak in (they would surface as the E0514 "multiple `core`" failure).
        sysroot_root = build_dir / 'brave-wasm-sysroot'
        if sysroot_root.exists():
            shutil.rmtree(sysroot_root)
        wasm_dir = (sysroot_root / 'lib' / 'rustlib' / WASM32_UNKNOWN_UNKNOWN)
        lib_dir = wasm_dir / 'lib'
        lib_dir.mkdir(parents=True)

        crates = [
            Path(part[1:].decode('utf-8'))
            for part in stamp.read_bytes().split(b'\0')
            if part and chr(part[0]) == 't'
        ]
        logging.info('Assembling %d wasm32 std crates from %s into %s',
                     len(crates), stamp, lib_dir)
        for crate in crates:
            shutil.copy2(crate, lib_dir / crate.name)

        # The `self-contained/` linker bits are the only part of the sysroot a
        # stage-0 build does populate; carry them over too.
        self_contained = (build_dir / STAGE0_RUSTLIB / WASM32_UNKNOWN_UNKNOWN /
                          'lib' / 'self-contained')
        if self_contained.is_dir():
            shutil.copytree(self_contained, lib_dir / 'self-contained')

        return wasm_dir

    def _chromium_version(self) -> str:
        """Parse the Chromium version from `chrome/VERSION` at HEAD.

        Returns the version as `MAJOR.MINOR.BUILD.PATCH`.
        """
        raw = _check_call('git',
                          'show',
                          f'HEAD:{CHROME_VERSION_FILE.as_posix()}',
                          cwd=self._chromium_src,
                          capture_output=True).stdout
        parts: dict[str, str] = {}
        for line in raw.splitlines():
            key, _, value = line.strip().partition('=')
            parts[key] = value
        return '{MAJOR}.{MINOR}.{BUILD}.{PATCH}'.format(**parts)

    def _chromium_commit(self) -> str:
        "Return the Chromium HEAD commit SHA (40-char hex)."
        return _check_call('git',
                           'rev-parse',
                           'HEAD',
                           cwd=self._chromium_src,
                           capture_output=True).stdout.strip()

    @staticmethod
    def _command_line() -> str:
        """Return the shell-quoted command line this script was invoked with.

        Reconstructed from `sys.argv` (the interpreter is not included), so it
        records exactly how the build was driven — the script name plus every
        flag. Stored in the index to make a build reproducible with the same
        invocation.
        """
        return shlex.join(sys.argv)

    def _upstream_stem(self) -> str:
        """Upstream toolchain package stem, i.e.
        `package_rust.RUST_TOOLCHAIN_PACKAGE_NAME` without the `.tar.xz`
        suffix.
        """
        return self._package_rust_module.RUST_TOOLCHAIN_PACKAGE_NAME.removesuffix(
            '.tar.xz')

    def _toolchain_name_stem(self) -> str:
        """Shared filename stem identifying this platform + Rust + Clang combo.
        """
        return f'{self._platform_prefix()}-{self._upstream_stem()}'

    def _package_name(self) -> str:
        """Return the filename for the output archive.

        Full naming schema:

            <platform>-rust-toolchain-<RUST_REVISION>-<RUST_SUB_REVISION>-
            llvmorg-<CLANG_REVISION>-<BRAVE_SUB_REVISION>.tar.xz

        Sections, left to right:

          1. `<platform>` — `linux-x64`, `mac`, etc.
          2. `rust-toolchain` — literal prefix, part of upstream's
             `package_rust.RUST_TOOLCHAIN_PACKAGE_NAME`.
          3. `<RUST_REVISION>` — upstream Rust commit SHA in `rust-lang/rust`,
             from `tools/rust/update_rust.py`.
          4. `<RUST_SUB_REVISION>` — upstream's manual respin counter, from
             `tools/rust/update_rust.py`.
          5. `llvmorg-<CLANG_REVISION>` — LLVM/Clang version Rust was built
             against, from `tools/clang/scripts/update.py`.
          6. `<BRAVE_SUB_REVISION>` — integer respin counter for this script,
             supplied via `--brave-subrevision`.
        """
        return f'{self._toolchain_name_stem()}-{self._brave_subrevision}.tar.xz'

    def _platform_prefix(self) -> str:
        """GCS-style platform prefix for the current host.

        This function  Mirrors the convention used by `package_rust.py` and
        `tools/clang/scripts/upload.sh`.

        The output is as follows:
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
                return 'mac-arm64'
            return 'mac'
        if sys.platform == 'win32':
            return 'win'
        return 'linux-x64'

    def _index_name(self) -> str:
        """Filename of the sibling YAML index for this exact build.

        Sibling of the output archive: shares `_package_name`'s full stem
        (including `<BRAVE_SUB_REVISION>`), so every respin gets its own index
        rather than all of them updating one shared file. See
        `toolchain_index_name`.
        """
        return toolchain_index_name(self._platform_prefix(),
                                    self._upstream_stem(),
                                    self._brave_subrevision)

    def _precheck_publishable(self) -> None:
        """Fail fast if this exact build's sibling index is already published.

        Guards against clobbering a toolchain already in use in the wild.
        Bump `--brave-subrevision` to publish a new respin instead.
        """
        index_url = f'{TOOLCHAIN_BUCKET_URL}/{self._index_name()}'
        if toolchain_publish.remote_url_exists(index_url):
            raise RuntimeError(
                f'{index_url} already exists; a toolchain for this exact '
                '--brave-subrevision is already published.')

    def _write_index(self, archive_path: Path) -> None:
        """Write the sibling YAML index describing the just-built archive.

        Writes one index per build, sharing the archive's name stem
        (`_index_name`) and holding a single mapping. The mapping has these
        fields:

          * `url`              — full bucket URL the tarball will be served at.
          * `timestamp`        — ISO 8601 UTC time of this build.
          * `sha256sum`        — hex SHA-256 of the tarball bytes.
          * `size_bytes`       — exact size of the tarball in bytes.
          * `chromium_version` — `MAJOR.MINOR.BUILD.PATCH` parsed from
                                 `chrome/VERSION`.
          * `chromium_commit`  — HEAD commit hash in the Chromium checkout.
          * `command_line`     — shell-quoted command line this script was
                                 invoked with (from `sys.argv`).
          * `brave_core_commit` — brave-core HEAD commit this builder ran from.

        The "already published" guard lives in `_precheck_publishable`, which
        `run()` calls early.
        """
        index_path = self._out_dir / self._index_name()

        index = {
            'url': f'{TOOLCHAIN_BUCKET_URL}/{archive_path.name}',
            'timestamp': datetime.now(timezone.utc).isoformat(),
            'sha256sum': sha256_file(archive_path),
            'size_bytes': archive_path.stat().st_size,
            'chromium_version': self._chromium_version(),
            'chromium_commit': self._chromium_commit(),
            'command_line': self._command_line(),
        }
        index['brave_core_commit'] = toolchain_publish.brave_core_commit()

        toolchain_publish.write_index_file(index_path, index)

    def _upload(self, archive_path: Path) -> None:
        """Upload the archive and its sibling index to the public bucket."""
        toolchain_publish.upload_files(
            TOOLCHAIN_BUCKET, TOOLCHAIN_BUCKET_PREFIX,
            (archive_path, self._out_dir / self._index_name()))

    def _package_full_rust(self) -> Path:
        """Build and package the full Rust toolchain via `package_rust.py`.

        Runs Chromium's `tools/rust/package_rust.py` (without `--upload`), which
        builds the complete toolchain — including bindgen and crubit — installs
        it to `RUST_TOOLCHAIN_OUT_DIR`, strips the binaries, and writes a
        `.tar.xz` to `third_party/`. The wasm32 sysroot is overlaid onto this
        archive afterwards by `_create_full_archive`.

        `--skip-test` is being passed because the test suite is causing the job
        to hang. This is most likely an issue between the way these tests work
        and the way jenkins is launching these processes without a TTY.

        Returns the absolute path of the toolchain archive produced under
        `third_party/` (`RUST_TOOLCHAIN_PACKAGE_NAME`).

        Raises:
            RuntimeError: if the expected archive is missing after the run.
        """
        _check_call(str(self._vpython_path),
                    'package_rust.py',
                    '--skip-test',
                    cwd=self._tools_rust)

        base_archive = (Path(self._build_rust_module.THIRD_PARTY_DIR) /
                        self._package_rust_module.RUST_TOOLCHAIN_PACKAGE_NAME)
        if not base_archive.is_file():
            raise RuntimeError(
                f'package_rust.py did not produce the expected archive at '
                f'{base_archive}')
        return base_archive

    def _create_full_archive(self, base_archive: Path, wasm_src: Path) -> Path:
        """Overlay the rust-lld + wasm32 artifacts onto the full-toolchain
        archive.

        Repacks `base_archive` (the full Rust toolchain produced by
        `_package_full_rust`) into `self._out_dir / _package_name()`, adding the
        same artifacts `_create_archive` ships, none of which `package_rust.py`
        installs into the toolchain tree:

        * `bin/rust-lld[.exe]` — the LLD linker from the Chromium LLVM install
          (`RUST_HOST_LLVM_INSTALL_DIR/bin/lld[.exe]`). `rustc` invokes
          `rust-lld` as the linker (e.g. for the wasm32 target), so the build
          fails with `linker 'rust-lld' not found` without it.
        * `lib/rustlib/wasm32-unknown-unknown/` — the wasm32 standard-library
          sysroot from the bootstrap build tree (`wasm_src`).
        * `bin/llvm-lib.exe` (Windows only) — the standalone MSVC-style
          librarian from the LLVM install.

        `.tar.xz` is a compressed stream and cannot be appended to in place, so
        every member of `base_archive` is copied across into a fresh archive
        before the overlay artifacts are added.

        Returns the absolute path of the archive on disk.
        """
        llvm_bin = Path(
            self._build_rust_module.RUST_HOST_LLVM_INSTALL_DIR) / 'bin'
        output_archive = self._out_dir / self._package_name()

        logging.info('Repacking %s into %s with the rust-lld + wasm32 overlay',
                     base_archive, output_archive)
        with tarfile.open(base_archive, 'r:xz') as src, \
                tarfile.open(output_archive,
                             'w:xz',
                             preset=9 | lzma.PRESET_EXTREME) as dst:
            for member in src.getmembers():
                fileobj = src.extractfile(member) if member.isreg() else None
                dst.addfile(member, fileobj)
            dst.add(llvm_bin / LLD, arcname=RUST_LLD_ARCNAME)
            dst.add(wasm_src, arcname=WASM32_ARCNAME)
            if sys.platform == 'win32':
                dst.add(llvm_bin / LLVM_LIB, arcname=LLVM_LIB_ARCNAME)
        return output_archive

    def _create_archive(self, wasm_src: Path) -> Path:
        """Write the output .tar.xz archive to `self._out_dir`.

        Returns the absolute path of the archive on disk.

        Members are laid out relative to a Rust toolchain root, mirroring the
        final on-disk layout under `src/third_party/rust-toolchain`, so the
        archive can be extracted straight over the toolchain directory:

        * `bin/rust-lld[.exe]` — the LLD linker binary from
          `RUST_HOST_LLVM_INSTALL_DIR/bin/lld[.exe]`.  Rust's toolchain ships
          its own copy of LLD under this name so that `rustc` can link without
          requiring a system linker.
        * `lib/rustlib/wasm32-unknown-unknown/` — the standard-library sysroot
          directory built by `_run_xpy` (`wasm_src`).  This directory contains
          the precompiled `core`, `alloc`, and `std` libraries needed to compile
          Rust code for the bare-metal WebAssembly target.
        * `bin/llvm-lib.exe` (Windows only) — the standalone MSVC-style
          librarian from `RUST_HOST_LLVM_INSTALL_DIR/bin/llvm-lib.exe`. Shipping
          this binary allows us to point AR straight at it, matching the
          upstream `tools/rust/config.toml.template` pattern
          (`ar = "$LLVM_BIN/llvm-lib.exe"`).
        """
        output_archive = self._out_dir / self._package_name()

        llvm_bin = Path(
            self._build_rust_module.RUST_HOST_LLVM_INSTALL_DIR) / 'bin'

        logging.info('Creating output archive at %s', output_archive)
        with tarfile.open(output_archive, 'w:xz') as tar:
            tar.add(llvm_bin / LLD, arcname=RUST_LLD_ARCNAME)
            tar.add(wasm_src, arcname=WASM32_ARCNAME)
            if sys.platform == 'win32':
                tar.add(llvm_bin / LLVM_LIB, arcname=LLVM_LIB_ARCNAME)
        return output_archive

    def _smoke_test_wasm(self, archive_path: Path) -> None:
        """Compile and link a wasm32 crate as a smoke test for the toolchain.

        Extracts `archive_path` over `RUST_TOOLCHAIN_OUT_DIR` exactly as a
        consumer would, then builds a small `std`-using crate for
        `wasm32-unknown-unknown` with that toolchain's `rustc` in two steps:

        * An rlib build loads `core`, `alloc` and `std` from the packaged
          sysroot, so a sysroot whose crates were built by a different `rustc`
          than the one consuming them fails with E0514 ("found crate `core`
          compiled by an incompatible version of rustc").
        * A cdylib build additionally links with the packaged `rust-lld`,
          exercising the shipped linker and the self-contained wasm objects.

        Either failure aborts before publishing, so a broken toolchain never
        reaches a downstream Brave build.

        Raises:
            RuntimeError: if the wasm32 crate fails to compile or link.
        """
        toolchain = Path(self._build_rust_module.RUST_TOOLCHAIN_OUT_DIR)
        wasm_sysroot = toolchain / 'lib' / 'rustlib' / WASM32_UNKNOWN_UNKNOWN

        # Drop any prior wasm32 overlay so stale crates can neither mask a real
        # mismatch nor fabricate one, then extract the archive as a consumer
        # would so `rustc` finds the shipped sysroot inside its own sysroot.
        if wasm_sysroot.exists():
            shutil.rmtree(wasm_sysroot)
        logging.info('Smoke test: extracting %s over %s', archive_path,
                     toolchain)
        with tarfile.open(archive_path, 'r:xz') as tar:
            tar.extractall(toolchain)

        rustc = toolchain / 'bin' / RUSTC
        # rust-lld ships in the toolchain's bin/; putting it on PATH is how the
        # real Brave build lets `rustc` find it for the wasm32 link step.
        link_env = {
            **os.environ,
            'PATH': os.pathsep.join(
                [str(toolchain / 'bin'), os.environ['PATH']]),
        }

        def _compile(label: str, crate_type: str, source: str, env=None):
            with tempfile.TemporaryDirectory() as tmp:
                src = Path(tmp) / 'wasm_smoke.rs'
                src.write_text(source, encoding='utf-8', newline='')
                out = Path(tmp) / 'wasm_smoke.out'
                logging.info('Smoke test: %s', label)
                try:
                    _check_call(str(rustc),
                                '--edition',
                                '2021',
                                '--target',
                                WASM32_UNKNOWN_UNKNOWN,
                                '--crate-type',
                                crate_type,
                                '-o',
                                str(out),
                                str(src),
                                env=env)
                except subprocess.CalledProcessError as e:
                    raise RuntimeError(
                        f'wasm32 toolchain smoke test failed ({label}): see the '
                        f'rustc error above. Refusing to publish.') from e

        # Compile-only only test.
        _compile('compiling a wasm32 rlib', 'rlib', WASM_SMOKE_RLIB_SRC)

        # Compile + link: drives the packaged rust-lld over the wasm objects.
        _compile('linking a wasm32 cdylib (rust-lld check)',
                 'cdylib',
                 WASM_SMOKE_CDYLIB_SRC,
                 env=link_env)

        logging.info('Smoke test passed: the wasm32 toolchain compiles and '
                     'links.')

    def _has_valid_chromium_path(self) -> bool:
        """Return whether self._chromium_src points to a valid Chromium repo."""
        # We start by checking for the presence of `chrome/VERSION`, as this is
        # a quite unmistakable Chromium repo trait that indicates a proper
        # checkout.
        if not (self._chromium_src / CHROME_VERSION_FILE).exists():
            return False

        logging.info('Checking for valid Chromium repo at %s',
                     self._chromium_src)
        try:
            _check_call('git',
                        'log',
                        '-1',
                        '--oneline',
                        str(CHROME_VERSION_FILE),
                        cwd=self._chromium_src)
        except (subprocess.CalledProcessError, OSError):
            return False

        return True

    def run(self, *, clear: bool, upload: bool, full_toolchain: bool,
            use_prebuilt_rustc: bool):
        """Execute the full build-and-package pipeline.

        Coordinates the phases in order:

        1. `_precheck_publishable` — fail fast if this exact
           `--brave-subrevision` is already published, before any of the
           (expensive) build steps below run.
        2. Within `cherry_picks` (which applies `CLANG_CHERRY_PICK_COMMITS`
           so both builds cherry-pick to identical hashes):
           a. `_package_full_rust` (only when `full_toolchain`) — build and
              package the complete Rust toolchain via `package_rust.py`.
           b. Within `_temporary_config_toml_template_edits`:
              - `_prepare_run_xpy` — clone, build LLVM, generate config.toml.
              - `_run_xpy` — compile the wasm32 stdlib via x.py, either building
                a stage-1 `rustc` or reusing the prebuilt one (see
                `--use-prebuilt-rustc`).
        3. Resolve the wasm32 sysroot, using `_assemble_stage0_wasm_sysroot` for
           the prebuilt path, `_stage1_wasm_stdlib_dir` otherwise, and assemble
           the output .tar.xz:
           * `_create_full_archive` when `full_toolchain` — overlay the wasm32
             sysroot onto the full-toolchain archive from step 1.
           * `_create_archive` otherwise — the minimal rust-lld + wasm32 subset.
        4. `_smoke_test_wasm` — compile a wasm32 crate against the packaged
           toolchain so a rustc/std mismatch fails before publishing.
        5. `_write_index` — write the sibling YAML index for the just-built
           archive.
        6. `_upload` (only with `--upload`) — publish the archive and its
           sibling index to `TOOLCHAIN_BUCKET_URL`.

        `config.toml.template` is returned to its original state always.

        Args:
            clear: If True, delete every entry under `self._out_dir` at the
                start of the run so the build produces output into a
                guaranteed-clean directory.
            upload: If True, upload the archive and its sibling index to
                `TOOLCHAIN_BUCKET_URL` after building (see `_upload`).
            full_toolchain: If True, package the whole Rust toolchain with
                `package_rust.py` and overlay the wasm32 sysroot onto it. If
                False (the default), package only the minimal rust-lld + wasm32
                subset via `_create_archive`.
            use_prebuilt_rustc: When True (the default), the
                `--no-full-toolchain` build compiles the wasm32 std against
                the prebuilt `rustc` gclient synced instead of building one
                from scratch. Ignored when `full_toolchain` is True, which
                always builds its own `rustc`.
        Raises:
            RuntimeError: If --chromium-src does not point at a valid Chromium
            checkout.
            RuntimeError: If tools_rust directory is not found.
            RuntimeError: If a toolchain for this exact `--brave-subrevision`
                is already published.
            RuntimeError: On Windows, if Git sh.exe is not in path and no
            installation for it can be found with Git.
            subprocess.CalledProcessError: If any subprocess command fails
                during the build process (from _prepare_run_xpy or _run_xpy).
        """
        if not self._has_valid_chromium_path():
            raise RuntimeError(
                '--chromium-src must point at an existing Chromium src '
                f'directory: {self._chromium_src}')

        if clear:
            logging.info('Clearing contents of %s', self._out_dir)
            shutil.rmtree(self._out_dir, ignore_errors=True)

        # Create the output directory.
        self._out_dir.mkdir(parents=True, exist_ok=True)

        # Validating that self._tools_rust is ready for use.
        if not self._tools_rust.is_dir():
            raise RuntimeError(f'{self._tools_rust} directory not found.')

        tools_rust_str: str = str(self._tools_rust)
        if tools_rust_str not in sys.path:
            sys.path.insert(0, tools_rust_str)
        if not self._build_rust_module:
            self._build_rust_module: ModuleType = importlib.import_module(
                'build_rust')
        if not self._package_rust_module:
            self._package_rust_module: ModuleType = importlib.import_module(
                'package_rust')

        # Fail fast, before any of the expensive build steps below, if this
        # exact respin is already published.
        self._precheck_publishable()

        # Build process
        if sys.platform == 'win32' and shutil.which('sh') is None:
            # Setting up git bin in PATH so `build_rust.py` can eventually
            # use sh.exe, which it requires to run.
            if GIT_SH_PRESUMED_BIN_PATH.is_file():
                logging.info(
                    'Adding Git bin to PATH for depot_tools on Windows: %s',
                    GIT_SH_PRESUMED_BIN_PATH.parent)
                os.environ['PATH'] = os.pathsep.join(
                    [str(GIT_SH_PRESUMED_BIN_PATH.parent), os.environ['PATH']])
            else:
                raise RuntimeError(
                    'Git sh.exe not found on PATH. This is required to run '
                    'build_rust.py on Windows. Please install Git for Windows '
                    'and ensure its bin/ directory is on PATH.')

        # `bootstrap` tool is cauing a linking warning on apple machines, that
        # becomes a hard error, because the build binds to the node's Homebrew
        # `liblzma.dylib` which targets a newer macOS than our deployment
        # target. For this reason, we force `lzma-sys` to compile and statically
        # link its bundled liblzma to avoid this warning. Upstream already does
        # this on Linux, but they have not hit this issue on macOS, as their
        # nodes don't have homebrew installed.
        if sys.platform == 'darwin':
            os.environ['LZMA_API_STATIC'] = '1'

        # Cargo should use git CLI for fetching, as there have been failures in
        # CI relating to ssh fetches.
        os.environ['CARGO_NET_GIT_FETCH_WITH_CLI'] = 'true'

        # In `--no-full-toolchain` mode, compile only the wasm32 std against the
        # prebuilt `rustc` gclient already synced rather than building one from
        # scratch (see `--use-prebuilt-rustc`). The full-toolchain build ships
        # its own `rustc` and so must build the wasm std alongside it.
        use_prebuilt_rustc = (use_prebuilt_rustc and not full_toolchain)

        # Cherry picks upstream commits (committership reproducibility fix and
        # any others) that the active Chromium ref may predate.
        with cherry_picks(self._chromium_src, CLANG_CHERRY_PICK_COMMITS):
            # Build and package the full Rust toolchain first, the overlay the
            # wasm32 sysroot onto it.
            base_archive = None
            if full_toolchain:
                base_archive = self._package_full_rust()

            with self._temporary_config_toml_template_edits():
                self._prepare_run_xpy()
                self._run_xpy(use_prebuilt_rustc=use_prebuilt_rustc)

        # The prebuilt-compiler path builds a bare `--stage 0` std that must be
        # assembled into a sysroot; the from-scratch path's `--stage 1` build
        # already assembled one.
        if use_prebuilt_rustc:
            wasm_src = self._assemble_stage0_wasm_sysroot()
        else:
            wasm_src = self._stage1_wasm_stdlib_dir()

        if full_toolchain:
            archive_path = self._create_full_archive(base_archive, wasm_src)
        else:
            archive_path = self._create_archive(wasm_src)

        # Verify the packaged toolchain compiles wasm32 before publishing.
        self._smoke_test_wasm(archive_path)

        self._write_index(archive_path)
        if upload:
            self._upload(archive_path)

        logging.info('Tarball download URL (once published): %s',
                     f'{TOOLCHAIN_BUCKET_URL}/{archive_path.name}')


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
    parser.add_argument(
        '--brave-subrevision',
        required=True,
        type=int,
        help='Integer respin counter, used to publish a sibling distinct '
        'archive.')
    parser.add_argument(
        '--clear',
        action='store_true',
        help='Makes sure the output directory is empty before building.')
    parser.add_argument(
        '--upload',
        action='store_true',
        help=f'Upload the archive and its sibling index to the public '
        f'build-deps bucket ({TOOLCHAIN_BUCKET}) after building.')
    # `--full-toolchain` / `--no-full-toolchain` are expressed as a pair of
    # store_true/store_false actions on a shared dest rather than
    # `argparse.BooleanOptionalAction`, which the presubmit's pylint does not
    # recognise.
    parser.add_argument(
        '--full-toolchain',
        dest='full_toolchain',
        action='store_true',
        default=False,
        help='Package the whole Rust toolchain via package_rust.py and overlay '
        'the wasm32 sysroot onto it. Without this flag only the minimal '
        'rust-lld + wasm32 subset is packaged (default).')
    parser.add_argument(
        '--no-full-toolchain',
        dest='full_toolchain',
        action='store_false',
        help='Package only the minimal rust-lld + wasm32 subset '
        'instead of the whole Rust toolchain.')
    parser.add_argument(
        '--use-prebuilt-rustc',
        dest='use_prebuilt_rustc',
        action='store_true',
        default=True,
        help='In --no-full-toolchain mode, compile the wasm32 std against the '
        'prebuilt rustc gclient synced rather than building one from scratch '
        '(default).')
    parser.add_argument(
        '--no-use-prebuilt-rustc',
        dest='use_prebuilt_rustc',
        action='store_false',
        help='In --no-full-toolchain mode, build a stage-1 rustc from scratch '
        'to compile the wasm32 std instead of reusing the prebuilt one.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging')
    args = parser.parse_args()

    if not args.chromium_src:
        parser.error('--chromium-src cannot be empty')
    if not args.out_dir:
        parser.error('--out-dir cannot be empty')

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    ToolchainBuilder(args.chromium_src,
                     args.out_dir,
                     brave_subrevision=args.brave_subrevision).run(
                         clear=args.clear,
                         upload=args.upload,
                         full_toolchain=args.full_toolchain,
                         use_prebuilt_rustc=(args.use_prebuilt_rustc))
    return 0


if __name__ == '__main__':
    sys.exit(main())
