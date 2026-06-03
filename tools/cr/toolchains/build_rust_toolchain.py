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

Keep this as a *standalone* script that can be invoked directly with a vanilla
Chromium checkout.

To use this straight from Github just call:

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/build_rust_toolchain.py \
    | vpython3 - \
        --out-dir=./out/ \
        --chromium-src=~/dev/chromium/src/ \
        --brave-subrevision=1
```

If you do not have a Chromium checkout yet, pass `--clone-chromium` together
with `--use-ref` to have the script fetch one automatically:

```sh
vpython3 build_rust_toolchain.py \
    --out-dir=./out/ \
    --chromium-src=~/dev/chromium/src/ \
    --clone-chromium \
    --use-ref=refs/heads/main \
    --brave-subrevision=1
```

The output of this script is a .tar.xz archive containing two artifacts built
against the Chromium-managed LLVM/Clang installation.  Members are stored at
paths relative to a Rust toolchain root, so the archive can be extracted
directly over `src/third_party/rust-toolchain`:

  * bin/rust-lld  — Rust's copy of the LLD linker, taken from the Chromium-built
                    LLVM install tree (`RUST_HOST_LLVM_INSTALL_DIR/bin/lld`).
  * lib/rustlib/wasm32-unknown-unknown  — the stage-1 standard-library sysroot
                              for the bare-metal WebAssembly target, taken from
                              the Rust bootstrap build tree
                              (`RUST_BUILD_DIR/<triple>/stage1/lib/rustlib/`).

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

Alongside the archive, a sibling YAML index is also written to `--out-dir`:

    <platform>-rust-toolchain-<RUST_REVISION>-<RUST_SUB_REVISION>
        -llvmorg-<CLANG_REVISION>.yaml

The index aggregates every build for the same platform and upstream Rust+Clang
combination — i.e. every distinct `<BRAVE_SUB_REVISION>` tarball under a single
file. The script downloads the existing index from the public toolchain bucket
(`TOOLCHAIN_BUCKET_URL`) or starts fresh if no prior index is published, appends
a new entry for the just-built tarball with the relevant metadata, and writes
the updated index to `--out-dir` for CI to upload back to the same path. Entries
are appended in chronological order. The last list element is the latest build
for the combo. The index has been introduced to preserve important information
about the toolchain build and reproducibility. It also can be used to query how
many versions we have for a given toolchain.

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
import hashlib
import importlib
import logging
from pathlib import Path
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tomllib
from types import ModuleType
import urllib.error
import urllib.request

import yaml

# Filename of the LLVM linker binary produced by the Chromium LLVM build.
LLD = 'lld' + ('.exe' if sys.platform == 'win32' else '')

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

# Relative path (within tools/rust/) of the Rust bootstrap configuration
# template. build_rust.py generates config.toml from this file.
CONFIG_TOML_TEMPLATE = Path('config.toml.template')

# Relative path (within the Chromium src/ root) of the Rust toolchain scripts.
TOOLS_RUST = Path('tools') / 'rust'

# Relative path (within RUST_BUILD_DIR/<target_triple>/) to the stage-1
# rustlib output directory.  The wasm32 standard-library sysroot lives at
# <RUST_BUILD_DIR>/<triple>/stage1/lib/rustlib/wasm32-unknown-unknown/.
STAGE1_RUSTLIB = Path('stage1') / 'lib' / 'rustlib'

# vpython3 that is selected by `depot_tools` from `$PATH`.
# This little Windows specific quirk is only needed when calling this script on
# Windows using git bash.
VPYTHON_PATH = Path('third_party/depot_tools') / (
    'vpython3.bat' if sys.platform == 'win32' else 'vpython3')

# Latest Chromium depot_tools bundle
DEPOT_TOOLS_URL = 'https://chromium.googlesource.com/chromium/tools/depot_tools'

# This source is used as a token to check if we have a valid Chromium repo as
# it is one of those reliable files that are always present in any version.
CHROME_VERSION_FILE = Path('chrome/VERSION')

# The bucket in our infra where the rust toolchain is archived.
TOOLCHAIN_BUCKET_URL = (
    'https://brave-build-deps-public.s3.brave.com/rust-toolchain-aux')

if sys.platform == 'win32':
    # Path to Git's sh.exe on Windows, which is used by
    # `tools/rust/build_rust.py` to build the toolchain on Windows.`
    GIT_SH_PRESUMED_BIN_PATH = Path(r'C:\Program Files\Git\bin\sh.exe')


def _check_call(*command, cwd=None):
    """Run *command* as a subprocess, logging the invocation.

    Logs the full command string at INFO level before executing it.  Stdout
    and stderr are inherited from the parent process so subprocess output
    streams directly to the terminal.

    Args:
        *command: The program and its arguments (passed as positional args,
            not as a list).
        cwd: Optional working directory for the subprocess.  Defaults to the
            caller's current working directory when `None`.

    Raises:
        subprocess.CalledProcessError: If the process exits with a non-zero
            return code.
        RuntimeError: On Windows, if the command cannot be resolved to an
        executable path.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))

    if platform.system() == 'Windows':
        # On Windows, resolve the command to an absolute path to avoid issues
        # with bat files not matching the command name (e.g. `gclient` vs
        # `gclient.bat`). This avoids the use of `shell=True`.
        resolved = shutil.which(command[0])
        if resolved is None:
            raise RuntimeError(f'Command not found: {command[0]}')
        if resolved != command[0]:
            command = [resolved] + list(command[1:])

    subprocess.run(command, cwd=cwd, check=True)


def _sha256_file(path: Path) -> str:
    """Return the hex SHA-256 of *path*'s bytes, computed in 64 KiB chunks.

    Streaming keeps a multi-hundred-megabyte tarball off the heap.
    """
    digest = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            digest.update(chunk)
    return digest.hexdigest()


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

    def __init__(self,
                 chromium_src: str,
                 out_dir: str,
                 brave_subrevision: int,
                 no_index_download: bool = False):
        """ Initialses the builder fields.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
            out_dir: Directory where the output `.tar.xz` archive is written.
            brave_subrevision: Integer respin counter encoded as the last
                section of the output archive name.  See
                `_package_name` for the full naming schema.
            no_index_download: If True, the publishing step looks for the
                prior index as a sibling file under `out_dir` instead of
                downloading it from the bucket.
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

        # When True, `_publish_archive_index` reads the prior index
        # from `self._out_dir` instead of fetching it from the bucket.
        self._no_index_download: bool = no_index_download

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
        target_triple: str = self._build_rust_module.RustTargetTriple()

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

    def _chromium_version(self) -> str:
        """Parse the Chromium version from `chrome/VERSION` at HEAD.

        Returns the version as `MAJOR.MINOR.BUILD.PATCH`.
        """
        raw = subprocess.check_output(
            ['git', 'show', f'HEAD:{CHROME_VERSION_FILE.as_posix()}'],
            cwd=self._chromium_src,
            text=True)
        parts: dict[str, str] = {}
        for line in raw.splitlines():
            key, _, value = line.strip().partition('=')
            parts[key] = value
        return '{MAJOR}.{MINOR}.{BUILD}.{PATCH}'.format(**parts)

    def _chromium_commit(self) -> str:
        "Return the Chromium HEAD commit SHA (40-char hex)."
        return subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                       cwd=self._chromium_src,
                                       text=True).strip()

    def _script_sha256(self) -> str | None:
        """SHA-256 of this script's source bytes, or `None` if unavailable.

        Returns `None` when `__file__` does not point at a real file. This can
        happen when the script is invoked via `curl ... | vpython3 -`, where
        `__file__` is the literal string `'<stdin>'`. This is not an issue in CI
        though because we use `vpython3 build_rust_toolchain.py`, which always
        creates a temp file.

        This function normalises the line endings to make sure we don't get
        different results based on platform.
        """
        file_str = globals().get('__file__')
        if not file_str:
            return None
        path = Path(file_str)
        if not path.is_file():
            return None
        # `encoding='utf-8'` causes CRLF/CR to be translated to LF on read
        return hashlib.sha256(
            path.read_text(encoding='utf-8').encode('utf-8')).hexdigest()

    def _toolchain_name_stem(self) -> str:
        """Shared filename stem identifying this platform + Rust + Clang combo.
        """
        upstream_stem = (self._package_rust_module.RUST_TOOLCHAIN_PACKAGE_NAME.
                         removesuffix('.tar.xz'))
        return f'{self._platform_prefix()}-{upstream_stem}'

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
        """Filename of the sibling YAML index for this Rust+Clang combo.

        Naming:

            <platform>-rust-toolchain-<RUST_REVISION>-<RUST_SUB_REVISION>
                -llvmorg-<CLANG_REVISION>.yaml
        """
        return f'{self._toolchain_name_stem()}.yaml'

    def _publish_archive_index(self,
                               archive_path: Path,
                               force_overwrite: bool = False) -> None:
        """Update the sibling YAML index after archive creation.

        This function is responsible for retrieving the existing index from the
        bucket, and updating it based on what we have for the current build. It
        also enforces the uniqueness of the archive, and preserves the relevant
        details for its reproducibility.

        The previous existing archive is downloaded, and the new updated archive
        is placed under `self._out_dir`, to be uploaded back to the bucket.

        Each entry in the index has these fields:

          * `url`              — full bucket URL the tarball will be served at.
          * `timestamp`        — ISO 8601 UTC time of this build.
          * `sha256sum`        — hex SHA-256 of the tarball bytes.
          * `chromium_version` — `MAJOR.MINOR.BUILD.PATCH` parsed from
                                 `chrome/VERSION`.
          * `chromium_commit`  — HEAD commit hash in the Chromium checkout.
          * `script_sha256sum` — SHA-256 of this script's contents.

        This function accepts `force_overwrite` for when we want to suspend
        certain rules, but the normal and preferred use of it will enforce these
        rules:

          - The produced toolchain should be unique. If the archive has another
            byte-identical toolchain already, a failure occurs.
          - The resulting URL must be unique. If the archive already contains a
            different toolchain at the same URL, a failure occurs.

        With `force_overwrite=True` (passed as `--force-overwrite`),
        both checks are bypassed: any entry at the same URL is
        removed before the new one is appended, and any matching
        `sha256sum` elsewhere in the index is tolerated — the index
        may end up with duplicate hashes if the operator deliberately
        publishes the same bytes twice.

        Collision matrix:

        +------------+---------------+--------------+---------------------+
        | URL match? | sha256 match? | Default      | --force-overwrite   |
        +============+===============+==============+=====================+
        | yes        | yes           | RuntimeError | replace old entry   |
        +------------+---------------+--------------+---------------------+
        | yes        | no            | RuntimeError | replace old entry   |
        +------------+---------------+--------------+---------------------+
        | no         | yes           | RuntimeError | allow duplicate     |
        +------------+---------------+--------------+---------------------+
        | no         | no            | append       | append              |
        +------------+---------------+--------------+---------------------+

        Entries are appended in chronological order. The newest entry for any
        tarball URL is always at the tail, so consumers can treat the last list
        element as the latest build for the given toolchain stem.

        NOTICE: This function relies on CI not running concurrent jobs for the
        same platform, and violating that will result in race conditions when
        updating the index.
        """
        index_name = self._index_name()
        local_index = self._out_dir / index_name
        archive_url = f'{TOOLCHAIN_BUCKET_URL}/{archive_path.name}'

        if self._no_index_download:
            if local_index.is_file():
                entries = yaml.safe_load(local_index.read_bytes()) or []
                logging.info(
                    'Loaded existing local index from %s (%d entries)',
                    local_index, len(entries))
            else:
                entries = []
                logging.info('No existing local index at %s; starting fresh',
                             local_index)
        else:
            index_url = f'{TOOLCHAIN_BUCKET_URL}/{index_name}'
            logging.info('Fetching %s', index_url)
            try:
                with urllib.request.urlopen(index_url) as response:
                    entries = yaml.safe_load(response) or []
                logging.info('Loaded existing index from %s (%d entries)',
                             index_url, len(entries))
            except urllib.error.HTTPError as e:
                # Unfortunately cloudfront seems to return 403 for cases where
                # it should have returned 404.
                if e.code not in (403, 404):
                    raise
                entries = []
                logging.info('No existing index at %s; starting fresh',
                             index_url)

        archive_sha256 = _sha256_file(archive_path)
        sha_match = next(
            (e for e in entries if e.get('sha256sum') == archive_sha256), None)
        url_match = next((e for e in entries if e.get('url') == archive_url),
                         None)

        # Collision is a hard failure: a byte-identical entry already exists
        # (sha256 match), or the target URL is already taken by different bytes
        # (URL match).
        if not force_overwrite:
            if sha_match is not None:
                raise RuntimeError(
                    f'Refusing to publish: a toolchain with sha256 '
                    f'{archive_sha256} is already in the index at '
                    f'{sha_match.get("url")!r}.  The just-built tarball '
                    f'is byte-identical; nothing new to publish.  Pass '
                    f'--force-overwrite to publish anyway.')
            if url_match is not None:
                raise RuntimeError(
                    f'Refusing to publish: an entry for {archive_url!r} '
                    f'already exists in the index (sha256 '
                    f'{url_match.get("sha256sum")!r}).  Pass '
                    f'--force-overwrite to replace it.')

        # With `--force-overwrite`, drop any prior entry at the same URL so the
        # new one cleanly replaces it.
        if force_overwrite and url_match is not None:
            logging.info(
                'Replacing existing index entry for %s '
                '(--force-overwrite in effect)', archive_url)
            entries = [e for e in entries if e.get('url') != archive_url]

        entry = {
            'url': archive_url,
            'timestamp': datetime.now(timezone.utc).isoformat(),
            'sha256sum': archive_sha256,
            'chromium_version': self._chromium_version(),
            'chromium_commit': self._chromium_commit(),
        }
        script_sha = self._script_sha256()
        if script_sha is not None:
            entry['script_sha256sum'] = script_sha
        entries.append(entry)

        index_yaml = yaml.safe_dump(entries,
                                    sort_keys=False,
                                    default_flow_style=False)
        local_index.write_text(index_yaml, encoding='utf-8', newline='')
        logging.info('Wrote toolchain index to %s (%d entries)', local_index,
                     len(entries))
        logging.info('Toolchain index contents:\n%s', index_yaml)

    def _create_archive(self) -> Path:
        """Write the output .tar.xz archive to `self._out_dir`.

        Returns the absolute path of the archive on disk.

        Members are laid out relative to a Rust toolchain root, mirroring the
        final on-disk layout under `src/third_party/rust-toolchain`, so the
        archive can be extracted straight over the toolchain directory:

        * `bin/rust-lld[.exe]` — the LLD linker binary from
          `RUST_HOST_LLVM_INSTALL_DIR/bin/lld[.exe]`.  Rust's toolchain ships
          its own copy of LLD under this name so that `rustc` can link without
          requiring a system linker.
        * `lib/rustlib/wasm32-unknown-unknown/` — the stage-1 standard-library
          sysroot directory located at
          `RUST_BUILD_DIR/<triple>/stage1/lib/rustlib/wasm32-unknown-unknown/`.
          This directory contains the precompiled `core`, `alloc`, and `std`
          libraries needed to compile Rust code for the bare-metal WebAssembly
          target.
        * `bin/llvm-lib.exe` (Windows only) — the standalone MSVC-style
          librarian from `RUST_HOST_LLVM_INSTALL_DIR/bin/llvm-lib.exe`. Shipping
          this binary allows us to point AR straight at it, matching the
          upstream `tools/rust/config.toml.template` pattern
          (`ar = "$LLVM_BIN/llvm-lib.exe"`).
        """
        target_triple = self._build_rust_module.RustTargetTriple()
        stage1_output_path = (Path(self._build_rust_module.RUST_BUILD_DIR) /
                              target_triple / STAGE1_RUSTLIB)
        output_archive = self._out_dir / self._package_name()

        llvm_bin = Path(
            self._build_rust_module.RUST_HOST_LLVM_INSTALL_DIR) / 'bin'

        logging.info('Creating output archive at %s', output_archive)
        with tarfile.open(output_archive, 'w:xz') as tar:
            tar.add(llvm_bin / LLD, arcname=RUST_LLD_ARCNAME)
            tar.add(stage1_output_path / WASM32_UNKNOWN_UNKNOWN,
                    arcname=WASM32_ARCNAME)
            if sys.platform == 'win32':
                tar.add(llvm_bin / LLVM_LIB, arcname=LLVM_LIB_ARCNAME)
        return output_archive

    def _bootstrap_depot_tools(self):
        """Ensure `depot_tools` is on PATH if no existing install is found.

        This method checks for `gclient` and, if it is not found, attempts to
        use or install `depot_tools` alongside `src/`.
        """
        if shutil.which('gclient') is not None:
            logging.debug('depot_tools already on PATH, skipping clone')
            return

        depot_tools_path = self._chromium_src.parent / 'depot_tools'
        if (depot_tools_path / 'gclient').is_file():
            # If `gclient` is already present in the expected install path, we
            # can skip cloning `depot_tools` and just add it to `PATH`.
            logging.info('depot_tools already present at %s, adding to PATH.',
                         depot_tools_path)
        else:
            logging.info('Installing depot_tools under %s', depot_tools_path)
            depot_tools_path.parent.mkdir(parents=True, exist_ok=True)
            _check_call('git', 'clone', DEPOT_TOOLS_URL, str(depot_tools_path))

        # Add depot_tools to PATH so that gclient can be used.
        os.environ['PATH'] = os.pathsep.join(
            [str(depot_tools_path), os.environ['PATH']])
        _check_call('gclient')

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

    def _checkout_chromium_ref(self, ref: str):
        """Check out a specific Chromium ref and resync dependencies."""
        logging.info('Checking out Chromium ref %s', ref)
        if (sys.platform == 'win32'
                and 'DEPOT_TOOLS_WIN_TOOLCHAIN' not in os.environ):
            # On Windows, we need the windows toolchain to be able to build the
            # code without a VS installation. This is desirable as we want
            # toolchains to be hermetic.
            os.environ["DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL"] = (
                'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-'
                '2.on.aws/windows-hermetic-toolchain/')

        if re.fullmatch(r'\d+\.\d+\.\d+\.\d+', ref):
            # Chromium release tag (e.g. `150.0.7850.1`): fetch it as a tag so
            # it lands at `refs/tags/<ref>` in the local repo.
            _check_call('git',
                        'fetch',
                        '--no-tags',
                        'origin',
                        f'refs/tags/{ref}:refs/tags/{ref}',
                        cwd=self._chromium_src)
        else:
            _check_call('git', 'fetch', 'origin', ref, cwd=self._chromium_src)

        # We are doing a `git checkout --force` rather than just using
        # `gclient sync -r {ref}`, as there is a an gclient bug that lurks
        # around which is not clear if we are hitting on the window bot, so for
        # the sake of preventing that to beging with, we do the checkout
        # manually.
        #
        # For details see:
        #   https://github.com/brave/brave-browser/issues/44921
        _check_call('git',
                    'checkout',
                    '--force',
                    'FETCH_HEAD',
                    cwd=self._chromium_src)

        _check_call('gclient', 'sync', '--force', '-D', cwd=self._chromium_src)
        _check_call('git',
                    'log',
                    '-1',
                    '--oneline',
                    str(CHROME_VERSION_FILE),
                    cwd=self._chromium_src)

    def _clone_chromium(self):
        """Clone a fresh Chromium checkout under `self._chromium_src.parent`."""
        if sys.platform == 'win32':
            # Setting up global git user.name and user.email is required to
            # avoid issues when building the rust toolchain on Windows, as it
            # is not uncommon for the WASM scripts to perform git operations
            # that fail if these are not set. As this can be surprisingly
            # disruptive, and mostly intended for CI, we guard this behind a
            # check for the presence of a user email, which indicates a
            # machine is not already set up as a developer environment.
            has_user_email = subprocess.run(
                ['git', 'config', '--global', '--get', 'user.email'],
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL).returncode == 0
            if not has_user_email:
                logging.warning('Setting global git user.name and user.email '
                                'to avoid clone failure on Windows. THIS WILL '
                                'OVERRIDE ANY PRE-EXISTING SETTINGS.')
                _check_call('git', 'config', '--global', 'user.name', 'Devops')
                _check_call('git', 'config', '--global', 'user.email',
                            'devops@brave.com')
                _check_call('git', 'config', '--global', 'core.autocrlf',
                            'false')
                _check_call('git', 'config', '--global', 'core.filemode',
                            'false')
                _check_call('git', 'config', '--global', 'core.preloadindex',
                            'true')
                _check_call('git', 'config', '--global', 'core.fscache',
                            'true')

        self._chromium_src.parent.mkdir(parents=True, exist_ok=True)
        _check_call('fetch',
                    '--nohooks',
                    'chromium',
                    cwd=self._chromium_src.parent)

    def run(self,
            clone_chromium: bool = False,
            use_ref: str = None,
            clear: bool = False,
            force_overwrite: bool = False):
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
            clone_chromium: Whether the builder should operate in a mode that
                allows cloning Chromium automatically, if necessary.
            clear: If True, delete every entry under `self._out_dir` at the
                start of the run so the build produces output into a
                guaranteed-clean directory.
            force_overwrite: If True, the publishing step bypasses both
                index-collision checks: an entry at the same URL is
                replaced rather than raising, and a matching `sha256sum`
                elsewhere is tolerated.  See `_publish_archive_index`.
        Raises:
            RuntimeError: If --chromium-src but there is no valid Chromium
            repo at the path, and --clone-chromium is not provided.
            RuntimeError: If tools_rust directory is not found.
            RuntimeError: On Windows, if Git sh.exe is not in path and no
            installation for it can be found with Git.
            subprocess.CalledProcessError: If any subprocess command fails
                during the build process (from _bootstrap_depot_tools,
                _clone_chromium, _checkout_chromium_ref, _prepare_run_xpy,
                or _run_xpy).
        """
        if not self._has_valid_chromium_path():
            if clone_chromium:
                self._bootstrap_depot_tools()
                logging.info('Chromium src not found at %s, cloning...',
                             self._chromium_src)
                self._clone_chromium()
            else:
                raise RuntimeError(
                    '--chromium-src must be an existing Chromium src '
                    f'directory: {self._chromium_src}')
        else:
            # We only try to bootstrap depot_tools when we have a Chromium
            # checkout (this case), or when we are trying to clone Chromium.
            self._bootstrap_depot_tools()

        if clear:
            logging.info('Clearing contents of %s', self._out_dir)
            shutil.rmtree(self._out_dir, ignore_errors=True)

        # Create the output directory.
        self._out_dir.mkdir(parents=True, exist_ok=True)

        # Validating that self._tools_rust is ready for use.
        if not self._tools_rust.is_dir():
            raise RuntimeError(f'{self._tools_rust} directory not found.')

        if use_ref:
            self._checkout_chromium_ref(use_ref)

        # Only loading the rust libs now that the desired git ref checkout is
        # done, otherwise the runtime would already be loaded with whatever rust
        # version values were in disk.
        tools_rust_str: str = str(self._tools_rust)
        if tools_rust_str not in sys.path:
            sys.path.insert(0, tools_rust_str)
        if not self._build_rust_module:
            self._build_rust_module: ModuleType = importlib.import_module(
                'build_rust')
        if not self._package_rust_module:
            self._package_rust_module: ModuleType = importlib.import_module(
                'package_rust')

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

        with self._temporary_config_toml_template_edits():
            self._prepare_run_xpy()
            self._run_xpy()

        archive_path = self._create_archive()
        self._publish_archive_index(archive_path,
                                    force_overwrite=force_overwrite)

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
    parser.add_argument('--clone-chromium',
                        action='store_true',
                        help='Allow cloning Chromium if needed')
    parser.add_argument(
        '--use-ref',
        help='Git reference (branch, tag, commit) to check out before building'
        ' the toolchain.')
    parser.add_argument(
        '--brave-subrevision',
        required=True,
        type=int,
        help='Integer respin counter, used to publish a sibling distinct '
        'archive.')
    parser.add_argument(
        '--no-index-download',
        action='store_true',
        help='Does not attempt to download the index from the bucket, but'
        'rather attempts to retrieve it from the output directory.')
    parser.add_argument(
        '--clear',
        action='store_true',
        help='Makes sure the output directory is empty before building.')
    parser.add_argument(
        '--force-overwrite',
        action='store_true',
        help='Bypasses the uniqueness checks for the toolchain being published '
        'and overwrites any entry on the index that collide with the toolchain '
        'being built.')
    parser.add_argument(
        '--with-git-cache',
        nargs='?',
        const='',
        metavar='PATH',
        help='Set GIT_CACHE_PATH in environment for the build (used by CI). '
        'Optionally provide PATH; if omitted, defaults to <home>/cache.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging')
    args = parser.parse_args()

    if not args.chromium_src:
        parser.error('--chromium-src cannot be empty')
    if not args.out_dir:
        parser.error('--out-dir cannot be empty')

    if args.clone_chromium and not args.use_ref:
        parser.error('--use-ref is required when --clone-chromium is provided')

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    if args.with_git_cache is not None:
        # This mirrors a bit what is being done in our infra, where the cache
        #  is usually baked under the home directory as `cache/`.
        if args.with_git_cache:
            git_cache_path = Path(args.with_git_cache).expanduser()
        else:
            if sys.platform == 'win32':
                git_cache_path = Path(
                    os.environ.get('USERPROFILE', str(Path.home()))) / 'cache'
            else:
                git_cache_path = Path(os.environ.get('HOME', str(
                    Path.home()))) / 'cache'

        if not git_cache_path.is_dir():
            raise RuntimeError(f'GIT_CACHE_PATH is not a valid directory: '
                               f'{git_cache_path}')
        if 'GIT_CACHE_PATH' in os.environ:
            raise RuntimeError('GIT_CACHE_PATH is already set in the '
                               'environment.')

        logging.info('Setting GIT_CACHE_PATH for the build: %s',
                     git_cache_path)
        os.environ['GIT_CACHE_PATH'] = str(git_cache_path)

    logging.info('Using GIT_CACHE_PATH=%s', os.environ.get('GIT_CACHE_PATH'))

    ToolchainBuilder(args.chromium_src,
                     args.out_dir,
                     brave_subrevision=args.brave_subrevision,
                     no_index_download=args.no_index_download).run(
                         args.clone_chromium,
                         args.use_ref,
                         clear=args.clear,
                         force_overwrite=args.force_overwrite)
    return 0


if __name__ == '__main__':
    sys.exit(main())
