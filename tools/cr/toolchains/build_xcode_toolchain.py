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
# wheel: <
#   name: "infra/python/wheels/rich-py3"
#   version: "version:13.2.0"
# >
# wheel: <
#   name: "infra/python/wheels/markdown-it-py-py3"
#   version: "version:2.1.0"
# >
# wheel: <
#   name: "infra/python/wheels/mdurl-py3"
#   version: "version:0.1.2"
# >
# wheel: <
#   name: "infra/python/wheels/pygments-py3"
#   version: "version:2.14.0"
# >
# [VPYTHON:END]
"""Build a hermetic, reproducible Xcode toolchain archive for Chromium.

Keep this as a *standalone* script that can be invoked directly on a macOS CI
node. PyYAML and Rich are the only non-stdlib imports and are provided by the
inline vpython3 spec above, so either use a vpython environment, or make sure to
install them into the system Python if running directly.

To run directly from GitHub on a worker that already has depot_tools on PATH:

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/build_xcode_toolchain.py \
    | vpython3 - \
        --out-dir=./out/ \
        --chromium-tag=150.0.7841.1
```

Or via the bootstrap forwarder on a fresh worker:

```sh
curl -sSLf \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/bootstrap_depot_tools.py \
    | python3 - \
        --run=https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/build_xcode_toolchain.py \
        -- \
        --out-dir=./out/ \
        --chromium-tag=150.0.7841.1
```

The script is intentionally self-contained: a CI invocation only needs a
Chromium tag and an output directory. It:

  1. Reads the macOS SDK build Chromium pins in `build/config/mac/mac_sdk.gni`
     at `--chromium-tag`, then consults `https://xcodereleases.com/api/all.json`
     to map that SDK build to the exact released Xcode (version, build and
     `.xip` filename) that shipped it.
  2. Deploys that Xcode: reuses one already expanded on the node (keyed by
     build under `/Applications`), or downloads the `.xip` from Brave's
     internal mirror, verifies it against the SHA-1 xcodereleases.com reports,
     expands it, and selects it with `xcode-select`.
  3. Clones the selected Xcode.app into a staging tree under `--out-dir` using
     BSD `cp -ac` (APFS clonefile, near-instant on the same volume).
  4. Downloads the on-demand Metal toolchain via
     `xcodebuild -downloadComponent metalToolchain` and rsyncs its
     artifacts into the staged `XcodeDefault.xctoolchain`.
  5. Fetches `build/xcode_binaries.yaml` from Chromium at `--chromium-tag` via
     gitiles.
  6. Packs the matching paths from the staged tree into a gzip-compressed
     tar with all per-entry metadata zeroed (mtime, uid, gid, uname, gname)
     and symlink targets normalized via `os.path.normpath`.
  7. Writes a sibling YAML index next to the archive, recording where the
     toolchain is served, its SHA-256, and the Xcode provenance. Publishing is
     refused if an index already exists for the toolchain unless
     `--force-overwrite` is given.

The output archive is written under `--out-dir` as
`xcode-hermetic-toolchain-<sdk-version>-<sdk-build>.tar.gz`, where the pair is
the SDK (`mac_sdk_official_version` / `mac_sdk_official_build_version`) that
Chromium pins in `build/config/mac/mac_sdk.gni` at `--chromium-tag`. Since the
builder deploys the exact Xcode that ships that SDK, this pair alone uniquely
identifies the toolchain.

The sibling index shares the archive's name stem (e.g.
`xcode-hermetic-toolchain-<sdk-version>-<sdk-build>.yaml`). It opens with the
Brave MPL license notice and records the archive `url`/`sha256sum`, the
`xcode_xip_source_url` (the Apple `.xip` URL from xcodereleases.com the
toolchain was built from) and its `xcode_xip_sha1sum`, the
`xcode_version`/`xcode_build`, the `metal_build`, the `chromium_tag`, and (when
run from a file) this script's `script_sha256sum`.

The full download URL is logged at the end of a successful run:
`<PACKAGE_DOWNLOAD_URL_BASE>/<archive-filename>`.
"""

from __future__ import annotations

import argparse
import base64
import dataclasses
import gzip
import hashlib
import json
import logging
import os
import re
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

import yaml
from rich.console import Console
from rich.progress import (BarColumn, DownloadColumn, Progress,
                           TaskProgressColumn, TextColumn, TimeRemainingColumn,
                           TransferSpeedColumn)
from rich.syntax import Syntax

# Gitiles raw-text endpoint for `build/xcode_binaries.yaml` at a given
# Chromium tag.
PKG_DEF_URL_TEMPLATE = (
    'https://chromium.googlesource.com/chromium/src/+/refs/tags/{tag}'
    '/build/xcode_binaries.yaml?format=TEXT')

# Gitiles raw-text endpoint for `build/config/mac/mac_sdk.gni` at a given
# Chromium tag. Provides the macOS SDK version triple that Chromium expects
# is using.
MAC_SDK_GNI_URL_TEMPLATE = (
    'https://chromium.googlesource.com/chromium/src/+/refs/tags/{tag}'
    '/build/config/mac/mac_sdk.gni?format=TEXT')

# Per-request timeout for the small HTTP fetches (gitiles raw files and the
# xcodereleases.com JSON).
HTTP_FETCH_TIMEOUT_SECS = 30

# Retry policy for gitiles fetches specifically: gitiles can flake on freshly
# pushed tags and sometimes takes a while to answer requests about them. Other
# small fetches (e.g. xcodereleases.com) are stable and are not retried.
GITILES_FETCH_MAX_ATTEMPTS = 3
GITILES_FETCH_RETRY_DELAY_SECS = 2

# Read timeout for the (multi-gigabyte) Xcode `.xip` download. Generous because
# it bounds individual socket reads against a large, slow transfer.
XIP_DOWNLOAD_TIMEOUT_SECS = 600

# Chunk size used to stream the Xcode `.xip` download and to hash it.
XIP_IO_CHUNK_BYTES = 1024 * 1024

# The base url used by brave to download the toolchain package. This is used in
# this script only to produce a log line with resulting URL.
PACKAGE_DOWNLOAD_URL_BASE = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws/'
    'xcode-hermetic-toolchain/')

# `xcodereleases.com` aggregates every Xcode release alongside the build
# numbers of the SDKs it bundles, and we use it as a source of truth for which
# Xcode version/build to use.
XCODE_RELEASES_API_URL = 'https://xcodereleases.com/api/all.json'

# A local cache of Xcode installs used in our infra (this bucket is not
# accessible to the public).
XCODE_ARCHIVE_BUCKET_URL = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws/'
    'xcode/')

# Directory where downloaded Xcodes are expanded and reused across runs, keyed
# by build number as `xcode_<build>.app`.
XCODE_APPS_DIR = Path('/Applications')

# Brave MPL license notice prepended to the generated YAML index, with the
# current year filled in at write time. YAML treats `#` lines as comments, so
# the index keeps loading cleanly while carrying the notice.
INDEX_LICENSE_HEADER_TEMPLATE = (
    '# Copyright (c) {year} The Brave Authors. All rights reserved.\n'
    '# This Source Code Form is subject to the terms of the Mozilla Public\n'
    '# License, v. 2.0. If a copy of the MPL was not distributed with this '
    'file,\n'
    '# You can obtain one at https://mozilla.org/MPL/2.0/.\n')


def _check_call(*command,
                cwd=None,
                capture_stdout: bool = False) -> str | None:
    """Run *command* as a subprocess, logging the invocation.

    Logs the full command string at INFO level before executing it.  Stderr is
    inherited from the parent process so subprocess output streams directly to
    the terminal.

    Args:
        *command: The program and its arguments (passed as positional args, not
            as a list).
        cwd: Optional working directory for the subprocess.  Defaults to the
            caller's current working directory when `None`.
        capture_stdout: When `True`, capture the child's stdout and return it as
            a UTF-8 decoded string.  When `False` (default), stdout is inherited
            from the parent and the function returns `None`.

    Returns:
        The decoded stdout if `capture_stdout=True`, otherwise `None`.

    Raises:
        subprocess.CalledProcessError: If the process exits with a non-zero
            return code.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))

    result = subprocess.run(command,
                            cwd=cwd,
                            check=True,
                            stdout=subprocess.PIPE if capture_stdout else None)

    if capture_stdout:
        return result.stdout.decode('utf-8', errors='replace')
    return None


def _fetch_gitiles_raw(url: str) -> str:
    """Fetch *url* (a gitiles `?format=TEXT` link) and decode its body.

    Gitiles' raw-text endpoint returns the requested file as a single
    base64-encoded blob with no surrounding HTML or headers.

    We attempt to recover a few times if gitlies is feeling temperamental.
    """
    for attempt in range(1, GITILES_FETCH_MAX_ATTEMPTS + 1):
        logging.info('Fetching %s (attempt %d/%d)', url, attempt,
                     GITILES_FETCH_MAX_ATTEMPTS)
        is_last_attempt = attempt == GITILES_FETCH_MAX_ATTEMPTS
        encoded: bytes | None = None
        try:
            with urllib.request.urlopen(
                    url, timeout=HTTP_FETCH_TIMEOUT_SECS) as response:
                encoded = response.read()
            return base64.b64decode(encoded).decode('utf-8')
        except urllib.error.HTTPError as e:
            # `HTTPError.read()` returns the response body. Let's log it.
            body = e.read().decode('utf-8', errors='replace')
            logging.error('HTTP %s on %s; response body:\n%s', e.code, url,
                          body)
            if is_last_attempt:
                raise
        except (urllib.error.URLError, TimeoutError) as e:
            # No response body for non-HTTP failures (DNS, connect timeout,
            # read timeout). Just surface the underlying reason.
            logging.error('Network error fetching %s: %s', url, e)
            if is_last_attempt:
                raise
        except ValueError as e:
            # Let's log the raw response whenever there are decoding issues.
            preview = (encoded or b'').decode('utf-8', errors='replace')
            logging.error('Decode failed for %s: %s; raw response:\n%s', url,
                          e, preview)
            if is_last_attempt:
                raise
        time.sleep(GITILES_FETCH_RETRY_DELAY_SECS)
    # Unreachable: the final attempt's except handlers always re-raise.
    # Present so every control-flow path honors the `-> str` signature
    # (pylint inconsistent-return-statements).
    raise RuntimeError(f'_fetch_gitiles_raw fell through retry loop: {url}')


def _fetch_xcode_releases() -> Any:
    """Fetch and parse the xcodereleases.com release catalog as JSON.
    """
    logging.info('Fetching %s', XCODE_RELEASES_API_URL)
    with urllib.request.urlopen(XCODE_RELEASES_API_URL,
                                timeout=HTTP_FETCH_TIMEOUT_SECS) as response:
        return json.loads(response.read().decode('utf-8'))


def _download_to_file(url: str, dest: Path) -> None:
    """Stream the response body from *url* into *dest* with a progress bar.

    Xcode `.xip` archives are multiple gigabytes, so the body is copied in
    chunks rather than read into memory. A self-updating progress bar is shown
    in the terminal, displaying percentage, size, throughput and ETA. The total
    is taken from the `Content-Length` header. If the server omits it the bar
    runs in indeterminate mode (size and speed only). Raises
    `urllib.error.URLError` (including `HTTPError`) on failure so the caller
    can fall back to an alternate URL.
    """
    with urllib.request.urlopen(
            url, timeout=XIP_DOWNLOAD_TIMEOUT_SECS) as response, \
         dest.open('wb') as out:
        total_bytes = int(response.headers.get('Content-Length') or 0)
        progress = Progress(
            TextColumn('[progress.description]{task.description}'),
            BarColumn(),
            TaskProgressColumn(),
            DownloadColumn(),
            TransferSpeedColumn(),
            TimeRemainingColumn(),
        )
        with progress:
            task = progress.add_task('Downloading Xcode',
                                     total=total_bytes or None)
            while chunk := response.read(XIP_IO_CHUNK_BYTES):
                out.write(chunk)
                progress.update(task, advance=len(chunk))


def _sha1_of_file(path: Path) -> str:
    """Return the hex SHA-1 digest of *path*, read in chunks.
    """
    digest = hashlib.sha1()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(XIP_IO_CHUNK_BYTES), b''):
            digest.update(chunk)
    return digest.hexdigest()


def _sha256_of_file(path: Path) -> str:
    """Return the hex SHA-256 digest of *path*, read in chunks.

    Used to fingerprint the output toolchain archive in the sibling index.
    """
    digest = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(XIP_IO_CHUNK_BYTES), b''):
            digest.update(chunk)
    return digest.hexdigest()


def _script_sha256() -> str | None:
    """Hex SHA-256 of this script's own source, or `None` if unavailable.

    `__file__` is absent (or not a real file) when the script is piped in, e.g.
    `curl ... | vpython3 -`, where there is nothing on disk to hash. The source
    is read in text mode so CRLF/CR are normalized to LF, keeping the digest
    identical regardless of the platform the file was checked out on.
    """
    file_str = globals().get('__file__')
    if not file_str:
        return None
    path = Path(file_str)
    if not path.is_file():
        return None
    return hashlib.sha256(
        path.read_text(encoding='utf-8').encode('utf-8')).hexdigest()


def _remote_url_exists(url: str) -> bool:
    """Return whether *url* already resolves to a published object.

    Treats HTTP 403/404 as "not published" -- the download bucket's CDN
    sometimes returns 403 where a 404 is expected. Any other HTTP status or a
    network error propagates, so a transient failure is never mistaken for
    "absent".
    """
    try:
        with urllib.request.urlopen(url, timeout=HTTP_FETCH_TIMEOUT_SECS):
            return True
    except urllib.error.HTTPError as e:
        if e.code in (403, 404):
            return False
        raise


def _normalize_tar_entry(tarinfo: tarfile.TarInfo) -> tarfile.TarInfo:
    """Strip host-specific metadata from a `TarInfo` before it is written.

    Zeroes mtime/uid/gid and clears uname/gname so the same Xcode tree on
    different builders produces byte-identical tar output. For symlinks, the
    link target is collapsed with `os.path.normpath`.

    File contents, file modes and entry types are left untouched: Mach-O code
    signatures depend on byte-exact preservation of binaries, and the
    executable bit on shipped compiler / linker binaries must survive into the
    archive.
    """
    tarinfo.mtime = 0
    tarinfo.uid = 0
    tarinfo.gid = 0
    tarinfo.uname = ''
    tarinfo.gname = ''
    if tarinfo.type == tarfile.SYMTYPE:
        tarinfo.linkname = os.path.normpath(tarinfo.linkname)
    return tarinfo


def _tree_size(path: Path) -> int:
    """Total bytes of the regular files at or under *path*.

    Mirrors what the archived members sum to so it can size the packing
    progress bar: symlinks and directories contribute 0, and symlinked
    directories are not descended (matching `TarFile.add`'s non-following
    behavior). A missing path contributes 0; the per-entry existence check in
    `_pack` is what turns that into a hard failure.
    """
    if path.is_symlink():
        return 0
    if path.is_file():
        return path.stat().st_size
    total = 0
    for root, _, files in os.walk(path):
        for name in files:
            entry = Path(root) / name
            if not entry.is_symlink() and entry.is_file():
                total += entry.stat().st_size
    return total


def _metal_build_from_path(metal_bin: str) -> str | None:
    """Metal toolchain build number from `xcrun --find metal`'s path.

    On recent macOS the Metal toolchain is a cryptex-mounted MobileAsset whose
    mount name carries its version, e.g.
    `...com.apple.MobileAsset.MetalToolchain-v17.5.188.0...`. Apple encodes the
    build (`17E188`) there as `<major>.<letter-index>.<build>`, so the numeric
    minor field maps back to the build letter (`5` -> `E`). Returns `None` if
    the path carries no recognizable Metal asset version.
    """
    match = re.search(r'MetalToolchain-v(\d+)\.(\d+)\.(\d+)', metal_bin)
    if not match:
        return None
    major, minor, build = (int(group) for group in match.groups())
    if not 1 <= minor <= 26:
        return None
    return f'{major}{chr(ord("A") + minor - 1)}{build}'


@dataclasses.dataclass(frozen=True)
class MacSdkInfo:
    """macOS SDK version triple reported by `xcodebuild -version -sdk macosx`.
    """

    # macOS SDK version, e.g. `26.5`.
    sdk_version: str

    # Product build number for that SDK, e.g. `25F70`.
    product_build_version: str


@dataclasses.dataclass(frozen=True)
class XcodeInfo:
    """Xcode app version and build reported by `xcodebuild -version`.
    """

    # Xcode version (e.g. `26.2`).
    version: str

    # Xcode build number (e.g. `25C57`).
    build: str


@dataclasses.dataclass(frozen=True)
class XcodeRelease:
    """A released Xcode resolved from `xcodereleases.com` by its SDK build."""

    # Xcode marketing version, e.g. `26.5`.
    version: str

    # Xcode build number, e.g. `17F42`.
    build: str

    # The original Apple `.xip` download URL as listed by xcodereleases.com,
    # e.g. `https://download.developer.apple.com/.../Xcode_26.5_Universal.xip`.
    download_url: str

    # The `.xip` filename, taken verbatim from `download_url`, e.g.
    # `Xcode_26.5_Universal.xip`.
    xip_filename: str

    # Expected SHA-1 of the `.xip`, from the xcodereleases `checksums.sha1`
    # field, used to verify the download. `None` if the catalog entry lists no
    # checksum, which `_resolve_xcode_release` rejects as unverifiable.
    sha1: str | None


class ToolchainBuilder:
    """A builder for the XCode hermetic toolchain archive.

    This builder does a few things:

    1. **Versioning** The builder reads the macOS SDK build Chromium pins for
       the given tag, resolves it to a released Xcode via `xcodereleases.com`
       (`_resolve_xcode_release`), deploys and selects that Xcode
       (`_install_xcode`, reusing or downloading + expanding its `.xip`), then
       reads back the active Xcode/SDK versions to confirm they match. The
       output archive is named solely after the upstream SDK pair, which alone
       identifies it because the deployed Xcode always ships that exact SDK.
    2. **Stage** (`_stage_xcode` + `_add_metal_toolchain`): Clones the
       detected Xcode.app into `self._staged_xcode` using BSD `cp -ac`
       (APFS clonefile), then downloads the on-demand Metal toolchain
       via `xcodebuild -downloadComponent metalToolchain` and rsyncs its
       artifacts into the staged `XcodeDefault.xctoolchain`.
    3. **Read** (`_fetch_pkg_def` / `_read_entries`): Fetches
       `xcode_binaries.yaml` from gitiles with the tag provided and stores the
       parsed `(kind, relpath)` tuples on `self._entries`.
    4. **Pack** (`_build_archive` / `_pack`): Streams every entry from
       `self._staged_xcode` into the output `.tar.gz` with
       `_normalize_tar_entry` applied so the bytes are reproducible
       across hosts.
    5. **Index** (`_precheck_publishable` / `_write_index`): Refuses early
       (right after reading the upstream SDK, before any heavy work) if an index
       is already published, unless `--force-overwrite` is set, then writes a
       sibling YAML index recording the archive URL, its SHA-256, and the
       Xcode provenance.
    """

    def __init__(self, chromium_tag: str, out_dir: Path):
        """Resolve the output directory and remember the requested tag.

        Args:
            chromium_tag: Chromium release tag (e.g. `150.0.7841.1`) used to
                fetch `build/xcode_binaries.yaml` and
                `build/config/mac/mac_sdk.gni` from gitiles.
            out_dir: Directory where the resulting `.tar.gz` (and a
                transient `staged_xcode/` working tree) are written.
        """
        # The Chromium tag being used. This value is used to for every download
        # from gitiles.
        self._chromium_tag: str = chromium_tag

        # Absolute path of the directory the output archive is written into.
        self._out_dir: Path = out_dir.expanduser().resolve()

        # Path to the local Xcode.app, populated by `_locate_xcode_app()`
        # from `xcode-select -p`.
        self._xcode_app: Path | None = None

        # Transient working tree where Xcode is cloned and augmented with
        # the Metal toolchain before `_build_archive` packs it. Created
        # by `_stage_xcode()` and removed by `run()` after a successful
        # archive build.
        self._staged_xcode: Path = self._out_dir / 'staged_xcode'

        # The macOS SDK version and build that Chromium is using. This is the
        # sole identifier encoded in the archive name, which lets brockit check
        # for the toolchain's existence from the SDK bump alone.
        self._upstream_mac_sdk_info: MacSdkInfo | None = None

        # The Xcode release resolved from `xcodereleases.com` for the upstream
        # macOS SDK build, populated by `_resolve_xcode_release()` and deployed
        # by `_install_xcode()`.
        self._xcode_release: XcodeRelease | None = None

        # Metal toolchain build number (e.g. `17E188`), populated by
        # `_add_metal_toolchain()` and recorded in the index. `None` if it
        # could not be determined.
        self._metal_build: str | None = None

        # Parsed `xcode_binaries.yaml` entries.
        self._entries: list[tuple[str, str]] = []

    @property
    def _archive_path(self) -> Path:
        """Path of the output archive.

        For the moment naming follows the simple convention:
          `xcode-hermetic-toolchain-<sdk-version>-<sdk-build>.tar.gz`

        If something more elaborate is required in the future (e.g. a Brave
        subversion), we can add it here, but for now this is enough to make sure
        we have a unique toolchain name of our own that can be queried from the
        values in `build/config/mac/mac_sdk.gni`.
        """
        if self._upstream_mac_sdk_info is None:
            raise RuntimeError(
                '_load_upstream_mac_sdk_info() must run before _archive_path')
        return self._out_dir / (
            'xcode-hermetic-toolchain'
            f'-{self._upstream_mac_sdk_info.sdk_version}'
            f'-{self._upstream_mac_sdk_info.product_build_version}'
            '.tar.gz')

    @property
    def _index_path(self) -> Path:
        """Path of the sibling YAML index for the output archive.

        The index shares the archive's name stem:
          `xcode-hermetic-toolchain-<sdk-version>-<sdk-build>.yaml`
        """
        archive = self._archive_path
        stem = archive.name.removesuffix('.tar.gz')
        return archive.with_name(f'{stem}.yaml')

    def _locate_xcode_app(self) -> None:
        """Resolve the local Xcode.app from the system developer dir.

        `xcode-select -p` prints the active developer directory, typically
        `/Applications/Xcode.app/Contents/Developer`. Walk two parents up
        to recover the .app bundle itself, then sanity-check the `.app`
        suffix so a misconfigured xcode-select (pointing at e.g.
        CommandLineTools) fails loud and early rather than producing a
        useless archive.
        """
        developer_dir = _check_call('xcode-select', '-p',
                                    capture_stdout=True).strip()
        app = Path(developer_dir).parent.parent
        if app.suffix != '.app':
            raise RuntimeError(
                f'xcode-select -p returned {developer_dir!r}; expected a '
                f'path inside an Xcode.app bundle (derived app={app}).')
        self._xcode_app = app
        logging.info('Detected Xcode at %s', self._xcode_app)

    def _stage_xcode(self) -> None:
        """Clone the detected Xcode.app into `self._staged_xcode`.

        Uses BSD `cp -ac`: `-a` is archive mode (preserve everything),
        `-c` enables APFS `clonefile()` so the copy is near-instant when
        source and destination share a volume. The trailing `/` on the
        source path means the *contents* of the .app are copied into the
        destination, so the staged tree begins at `Contents/...` —
        matching the layout `_pack` and Chromium's `xcode_binaries.yaml`
        expect.
        """
        assert self._xcode_app is not None
        if self._staged_xcode.exists():
            logging.info('Removing existing stage at %s', self._staged_xcode)
            shutil.rmtree(self._staged_xcode)
        self._staged_xcode.mkdir(parents=True)
        _check_call('cp', '-ac', f'{self._xcode_app}/',
                    str(self._staged_xcode))

    def _add_metal_toolchain(self) -> None:
        """Download the Metal toolchain and graft it into the staged Xcode.

        Recent Xcode releases stopped shipping the Metal compiler inside the
        `.app` bundle. It is an on-demand component installed via
        `xcodebuild -downloadComponent metalToolchain` into a system-wide cache.
        Once present, `xcrun --find metal` resolves the `metal` binary. We
        truncate that path back to the enclosing `Metal.xctoolchain` directory
        and rsync the relevant subtrees into the staged
        `XcodeDefault.xctoolchain` so the packager picks them up. The
        toolchain's build number is also recovered from that path and stored on
        `self._metal_build` for the index.
        """
        _check_call('xcodebuild', '-downloadComponent', 'metalToolchain')
        metal_bin = _check_call('xcrun',
                                '--find',
                                'metal',
                                capture_stdout=True).strip()
        self._metal_build = _metal_build_from_path(metal_bin)
        logging.info('Metal toolchain build: %s', self._metal_build
                     or '(unknown)')
        match = re.search(r'^(.*/Metal\.xctoolchain)/', metal_bin)
        if not match:
            raise RuntimeError(
                'Could not derive Metal.xctoolchain from `xcrun --find '
                f'metal` output: {metal_bin!r}')
        metal_dir = Path(match.group(1))
        dest = (self._staged_xcode /
                'Contents/Developer/Toolchains/XcodeDefault.xctoolchain' /
                'usr')
        # Mirror the `usr/metal/` tree wholesale, including deletes — the
        # staged copy may already contain a stale Metal install.
        _check_call('rsync', '--archive', '--delete',
                    f'{metal_dir}/usr/metal/', f'{dest}/metal')
        # Add the individual driver binaries into `usr/bin/`. No --delete
        # here: we are augmenting an existing bin/ directory, not mirroring.
        _check_call('rsync', '--archive', f'{metal_dir}/usr/bin/air-lld',
                    f'{metal_dir}/usr/bin/metal',
                    f'{metal_dir}/usr/bin/metallib', f'{dest}/bin/')

    def _load_local_versions(self) -> None:
        """Probe `xcodebuild` for the active Xcode and macOS SDK versions.

        Runs `xcodebuild -version -sdk macosx`, whose output carries both the
        Xcode-level lines (`Xcode <ver>` / `Build version <build>`) and the SDK-
        level lines (`SDKVersion:` / `ProductBuildVersion:`). All four values
        are load-bearing: the Xcode build is checked against the resolved
        release and the SDK pair against `self._upstream_mac_sdk_info`, so a
        wrong active Xcode/SDK fails the build. Missing any one of them is a
        hard failure rather than a silent fallback.
        """
        output = _check_call('xcodebuild',
                             '-version',
                             '-sdk',
                             'macosx',
                             capture_stdout=True)
        sdk_version = re.search(r'^SDKVersion: (.+)$', output, re.MULTILINE)
        sdk_build = re.search(r'^ProductBuildVersion: (.+)$', output,
                              re.MULTILINE)

        output = _check_call('xcodebuild', '-version', capture_stdout=True)
        xcode_version = re.search(r'^Xcode (.+)$', output, re.MULTILINE)
        xcode_build = re.search(r'^Build version (.+)$', output, re.MULTILINE)

        if not (xcode_version and xcode_build and sdk_version and sdk_build):
            raise RuntimeError(
                'xcodebuild did not report all of Xcode / Build version / '
                f'SDKVersion / ProductBuildVersion; raw output:\n{output}')
        local_xcode_info = XcodeInfo(version=xcode_version.group(1).strip(),
                                     build=xcode_build.group(1).strip())
        local_mac_sdk_info = MacSdkInfo(
            sdk_version=sdk_version.group(1).strip(),
            product_build_version=sdk_build.group(1).strip())

        # Invariants by this point: `run()` resolves the upstream SDK and the
        # Xcode release before deploying/probing, so both must be populated.
        assert self._xcode_release is not None
        assert self._upstream_mac_sdk_info is not None

        # `_install_xcode()` should have selected the Xcode we resolved. If the
        # selected Xcode build doesn't match the one we are expected to be
        # using to build the SDK, we must fail the run.
        if local_xcode_info.build != self._xcode_release.build:
            raise RuntimeError(
                f'Active Xcode build {local_xcode_info.build} does not '
                f'match the resolved build {self._xcode_release.build}; '
                'xcode-select may be pointing at the wrong Xcode.')

        # The deployed Xcode must ship the exact macOS SDK Chromium, otherwise
        # the build would be using an incorrect SDK.
        if local_mac_sdk_info != self._upstream_mac_sdk_info:
            raise RuntimeError(
                'Active macOS SDK '
                f'{local_mac_sdk_info.sdk_version} '
                f'(build {local_mac_sdk_info.product_build_version}) '
                'does not match the upstream-pinned '
                f'{self._upstream_mac_sdk_info.sdk_version} '
                f'(build {self._upstream_mac_sdk_info.product_build_version}).'
            )

        logging.info('Local Xcode: %s (build %s)', local_xcode_info.version,
                     local_xcode_info.build)
        logging.info('Local macOS SDK: %s (build %s)',
                     local_mac_sdk_info.sdk_version,
                     local_mac_sdk_info.product_build_version)

    def _load_upstream_mac_sdk_info(self) -> None:
        """Fetch the macOS SDK used in the Chromium tag provided.

        Reads `build/config/mac/mac_sdk.gni` from gitiles and pulls the
        `mac_sdk_official_version` and `mac_sdk_official_build_version`
        from these sources, storing them in their corresponding fields.
        """
        url = MAC_SDK_GNI_URL_TEMPLATE.format(tag=self._chromium_tag)
        text = _fetch_gitiles_raw(url)

        def gn_value(key: str) -> str:
            # gn auto-formats assignments as `key = "value"`.
            match = re.search(rf'{key} = "([^"]+)"', text)
            if not match:
                raise RuntimeError(f'{key} not found in {url}')
            return match.group(1)

        self._upstream_mac_sdk_info = MacSdkInfo(
            sdk_version=gn_value('mac_sdk_official_version'),
            product_build_version=gn_value('mac_sdk_official_build_version'))
        logging.info('Upstream macOS SDK version: %s (build %s)',
                     self._upstream_mac_sdk_info.sdk_version,
                     self._upstream_mac_sdk_info.product_build_version)

    def _resolve_xcode_release(self) -> None:
        """Map the upstream macOS SDK build to a released Xcode `.xip`.

        Queries `xcodereleases.com` and looks for the *released* (not beta, not
        RC) Xcode whose bundled macOS SDK build matches the upstream SDK build.
        A single SDK build is shared by several catalog entries (the Universal
        package, the Apple-silicon-only package, and the matching release
        candidates), so the result is filtered down to the released Universal
        archive, which is what we use in our infra. Stores the result on
        `self._xcode_release`.
        """
        assert self._upstream_mac_sdk_info is not None
        target_build = self._upstream_mac_sdk_info.product_build_version
        data = _fetch_xcode_releases()

        candidates: list[XcodeRelease] = []
        for entry in data:
            version = entry.get('version') or {}
            # Only final releases: the `release` object carries a truthy
            # `release` key. Betas (`{'beta': N}`), release candidates
            # (`{'rc': N}`), GM seeds, etc. all lack it and are skipped.
            if not (version.get('release') or {}).get('release'):
                continue
            macos_sdks = (entry.get('sdks') or {}).get('macOS') or []
            if not any(sdk.get('build') == target_build for sdk in macos_sdks):
                continue
            url = ((entry.get('links') or {}).get('download') or {}).get('url')
            if not url:
                continue
            candidates.append(
                XcodeRelease(version=version.get('number'),
                             build=version.get('build'),
                             download_url=url,
                             xip_filename=url.rsplit('/', 1)[-1],
                             sha1=(entry.get('checksums') or {}).get('sha1')))

        # Brave mirrors the Universal package. That's the default. Only if no
        # Universal archive is listed do we consider the remaining
        # (e.g. arm64-only) candidates.
        universal = [c for c in candidates if 'Universal' in c.xip_filename]
        chosen = universal or candidates
        if not chosen:
            raise RuntimeError(
                f'No released Xcode on {XCODE_RELEASES_API_URL} bundles macOS '
                f'SDK build {target_build}')
        if len(chosen) > 1:
            names = ', '.join(c.xip_filename for c in chosen)
            raise RuntimeError(
                'Ambiguous Xcode release: multiple released archives bundle '
                f'macOS SDK build {target_build}: {names}')
        self._xcode_release = chosen[0]
        if not self._xcode_release.sha1:
            raise RuntimeError(
                f'{XCODE_RELEASES_API_URL} lists no SHA-1 checksum for '
                f'{self._xcode_release.xip_filename}; refusing to install an '
                'unverifiable Xcode archive.')
        logging.info('Resolved Xcode %s (build %s) -> %s (sha1 %s)',
                     self._xcode_release.version, self._xcode_release.build,
                     self._xcode_release.xip_filename,
                     self._xcode_release.sha1)

    def _install_xcode(self) -> None:
        """Deploy and select the resolved Xcode, installing if needed.

        Reuses an Xcode previously expanded on this node, keyed by build number,
        under `XCODE_APPS_DIR` as `xcode_<build>.app`. If it is not already
        present, the `.xip` is downloadedand expanded into place. Either way the
        chosen Xcode is then made active via `xcode-select`.
        """
        assert self._xcode_release is not None
        app_name = f'xcode_{self._xcode_release.build.lower()}.app'
        app_path = XCODE_APPS_DIR / app_name
        if app_path.exists():
            logging.info('Reusing Xcode already expanded at %s', app_path)
        else:
            self._download_and_expand_xcode(app_path)
        self._select_xcode(app_path)

    def _download_and_expand_xcode(self, app_path: Path) -> None:
        """Download the resolved `.xip` and expand it to *app_path*.

        Fetches the archive into `--out-dir`, verifies it against its expected
        SHA-1 provided by `xcodereleases.com`, then expands the `xip` archive,
        and finally the resulting `Xcode.app` is moved into `app_path`. The
        expansion happens on the same volume as `app_path` so the final move is
        an instantaneous rename rather than a multi-gigabyte copy.
        """
        assert self._xcode_release is not None
        self._out_dir.mkdir(parents=True, exist_ok=True)
        xip_path = self._out_dir / self._xcode_release.xip_filename
        self._download_xip(xip_path)

        # Expand into a sibling of the destination (same volume) so the final
        # `Xcode.app` -> `xcode_<build>.app` move is a rename, not a copy.
        expand_dir = app_path.with_name(f'.{app_path.stem}.expand')
        if expand_dir.exists():
            shutil.rmtree(expand_dir)
        expand_dir.mkdir(parents=True)
        try:
            # `unxip` and `xip --expand` both write `Xcode.app` into the current
            # directory and take no output-path flag, hence the dedicated `cwd`.
            # `unxip` is dramatically faster, but is not installed by default,
            # so prefer it when present and fall back to Apple's `xip`
            # otherwise.
            if shutil.which('unxip'):
                _check_call('unxip', str(xip_path), cwd=expand_dir)
            else:
                logging.warning(
                    '`unxip` not found on PATH; falling back to the slower '
                    '`xip --expand`. Installing `unxip` (`brew install unxip`) '
                    'is strongly preferred for much faster Xcode expansion.')
                _check_call('xip', '--expand', str(xip_path), cwd=expand_dir)
            expanded_app = expand_dir / 'Xcode.app'
            if not expanded_app.exists():
                raise RuntimeError(
                    f'xip did not produce Xcode.app under {expand_dir}')
            logging.info('Installing expanded Xcode to %s', app_path)
            shutil.move(expanded_app, app_path)
        finally:
            shutil.rmtree(expand_dir, ignore_errors=True)
            xip_path.unlink(missing_ok=True)

    def _download_xip(self, dest: Path) -> None:
        """Download the resolved, SHA-1-verified `.xip` into *dest*.

        Tries the published filename first, then a variant with the `_Universal`
        segment stripped (DevOps sometimes strips that portion from the name).
        Each download is checked against the `checksums.sha1` value
        xcodereleases.com reports for the release. An unverifiable archive is
        never left on disk.
        """
        assert self._xcode_release is not None
        # `expected_sha1` is guaranteed non-None by `_resolve_xcode_release`.
        expected_sha1 = self._xcode_release.sha1
        assert expected_sha1 is not None
        filename = self._xcode_release.xip_filename
        urls = [XCODE_ARCHIVE_BUCKET_URL + filename]
        stripped = filename.replace('_Universal', '')
        if stripped != filename:
            urls.append(XCODE_ARCHIVE_BUCKET_URL + stripped)

        last_error: Exception | None = None
        for url in urls:
            try:
                logging.info('Downloading Xcode archive %s', url)
                _download_to_file(url, dest)
            except urllib.error.URLError as e:
                logging.warning('Download failed for %s: %s', url, e)
                last_error = e
                continue
            logging.info('Verifying SHA-1 of %s', dest)
            actual_sha1 = _sha1_of_file(dest)
            if actual_sha1 != expected_sha1:
                logging.warning('SHA-1 mismatch for %s: expected %s, got %s',
                                url, expected_sha1, actual_sha1)
                dest.unlink(missing_ok=True)
                last_error = RuntimeError(
                    f'SHA-1 mismatch for {url}: expected {expected_sha1}, '
                    f'got {actual_sha1}')
                continue
            logging.info('Verified SHA-1 %s of %s', actual_sha1, dest)
            return
        raise RuntimeError(
            f'Could not download a verified Xcode archive. Tried {urls}'
        ) from last_error

    def _select_xcode(self, app_path: Path) -> None:
        """Make *app_path* the active Xcode via `xcode-select`.

        Switches the developer dir with `sudo xcode-select -s`, and runs
        `xcodebuild -runFirstLaunch` so the freshly expanded Xcode finishes its
        one-time setup.
        """
        _check_call('sudo', '/usr/bin/xcode-select', '-s', str(app_path))
        _check_call('sudo', '/usr/bin/xcodebuild', '-license', 'accept')
        _check_call('sudo', '/usr/bin/xcodebuild', '-runFirstLaunch')

        # This is to avoid issues caused by mixed usage of different Xcode
        # versions on one machine.
        _check_call('xcrun', 'simctl', 'list')

    def _fetch_pkg_def(self) -> str:
        """Fetch `xcode_binaries.yaml` from gitiles."""
        return _fetch_gitiles_raw(
            PKG_DEF_URL_TEMPLATE.format(tag=self._chromium_tag))

    def _read_entries(self) -> None:
        """Load the `data:` list from `xcode_binaries.yaml` in document order.

        Each entry is stored as `(kind, relpath)` on `self._entries`, where
        `kind` is `'file'` or `'dir'`. Top-level keys other than `data:` and
        per-entry keys other than `file:` / `dir:` are ignored. The upstream
        YAML is trusted input and is loaded with `yaml.Loader`; Chromium pkg
        defs may include Python-tagged values that `yaml.safe_load` would
        refuse.
        """
        text = self._fetch_pkg_def()
        pkg = yaml.load(text, Loader=yaml.Loader) or {}
        entries: list[tuple[str, str]] = []
        for entry in pkg.get('data') or []:
            if file_path := entry.get('file'):
                entries.append(('file', file_path))
            elif dir_path := entry.get('dir'):
                entries.append(('dir', dir_path))
        self._entries = entries

    def _pack(self, tar: tarfile.TarFile) -> None:
        """Add every YAML entry to `tar` in YAML document order.

        Archive member names are the relpaths exactly as they appear in the
        YAML; recursion into directories appends path components beneath.
        `TarFile.add` enumerates directory children in sorted basename order
        (CPython sorts `os.listdir` results), which is what bit-reproducible
        output requires across hosts (filesystem readdir order is not stable
        across macOS versions or APFS snapshots).

        A missing `file:` or `dir:` entry is a hard failure -- the script
        makes no attempt to best-effort around a malformed Xcode install.

        A live progress bar tracks bytes packed: the total is the pre-computed
        size of the listed entries, and the per-member `filter` callback
        advances it by each member's `TarInfo.size` as it is written.
        """
        total_bytes = sum(
            _tree_size(self._staged_xcode / relpath)
            for _, relpath in self._entries)
        progress = Progress(
            TextColumn('[progress.description]{task.description}'),
            BarColumn(),
            TaskProgressColumn(),
            DownloadColumn(),
            TransferSpeedColumn(),
            TimeRemainingColumn(),
        )
        with progress:
            task = progress.add_task('Packing toolchain', total=total_bytes)

            def add_member(tarinfo: tarfile.TarInfo) -> tarfile.TarInfo:
                tarinfo = _normalize_tar_entry(tarinfo)
                progress.update(task, advance=tarinfo.size)
                return tarinfo

            for kind, relpath in self._entries:
                source = self._staged_xcode / relpath
                # `is_symlink()` catches dangling symlinks `exists()` skips.
                if not source.exists() and not source.is_symlink():
                    raise FileNotFoundError(
                        f'pkg_def {kind}: {relpath!r} not found under '
                        f'{self._staged_xcode}')
                logging.debug('+ %s', relpath)
                tar.add(source, arcname=relpath, filter=add_member)

    def _build_archive(self) -> None:
        """Write the deterministic `.tar.gz` under `self._out_dir`.

        The gzip outer layer is opened with `mtime=0` and an empty `filename` so
        the gzipped bytes on disk are reproducible across runs (the tar payload
        is already reproducible by virtue of per-entry normalization). PAX
        format is selected so long member names and UTF-8 metadata are encoded
        portably.
        """
        output = self._archive_path
        logging.info('Packing %d entries from %s into %s', len(self._entries),
                     self._staged_xcode, output)
        with output.open('wb') as raw_fp, \
             gzip.GzipFile(filename='', mode='wb', fileobj=raw_fp,
                           mtime=0) as gz, \
             tarfile.open(fileobj=gz, mode='w',
                          format=tarfile.PAX_FORMAT) as tar:
            self._pack(tar)

    def _precheck_publishable(self) -> None:
        """Fail fast if this toolchain's index is already published.

        This function check for the existence of an index already in place, and
        if any is found, it throws an error.

        At the moment, the index can only refer to a single toolchain per SDK
        version/build pair, so checking for the file existence in the bucket is
        enough.

        This check is important to avoid accidentally overwriting a toolchain
        that is already in use by Brave in the wild, as it can become incredibly
        difficult to recover from such a mistake.
        """
        index_url = PACKAGE_DOWNLOAD_URL_BASE + self._index_path.name
        if _remote_url_exists(index_url):
            raise RuntimeError(
                f'An index already exists at {index_url}; this toolchain has '
                'already been published. Pass --force-overwrite to rebuild and '
                'replace it.')

    def _write_index(self) -> None:
        """Write the sibling YAML index describing the just-built toolchain.

        This file has the following entries:

          * `url`              — bucket URL the toolchain archive is served on.
          * `sha256sum`        — hex SHA-256 of the archive bytes.
          * `xcode_xip_source_url` — Apple `.xip` URL from xcodereleases.com
                                 the toolchain was built from.
          * `xcode_xip_sha1sum` — SHA-1 of that `.xip` (from xcodereleases.com,
                                 verified against the download).
          * `xcode_version`    — Xcode marketing version, e.g. `26.5`.
          * `xcode_build`      — Xcode build number, e.g. `17F42`.
          * `metal_build`      — Metal toolchain build, e.g. `17E188` (`null`
                                 if it could not be determined).
          * `chromium_tag`     — `--chromium-tag` the SDK pin was read from.
          * `script_sha256sum` — SHA-256 of this script's source, included only
                                 when run from a file on disk (see
                                 `_script_sha256`).

        The "already published" guard lives in `_precheck_publishable`, which
        `run()` calls early. If we got here, and we are overwriting the previous
        file, this means clobbering was allowed with `--force-overwrite`.

        After writing, the index file is read back and printed in the console.
        """
        assert self._xcode_release is not None
        archive_path = self._archive_path
        index_path = self._index_path

        index = {
            'url': PACKAGE_DOWNLOAD_URL_BASE + archive_path.name,
            'sha256sum': _sha256_of_file(archive_path),
            'xcode_xip_source_url': self._xcode_release.download_url,
            'xcode_xip_sha1sum': self._xcode_release.sha1,
            'xcode_version': self._xcode_release.version,
            'xcode_build': self._xcode_release.build,
            'metal_build': self._metal_build,
            'chromium_tag': self._chromium_tag,
        }
        script_sha256 = _script_sha256()
        if script_sha256 is not None:
            index['script_sha256sum'] = script_sha256
        index_yaml = yaml.safe_dump(index,
                                    sort_keys=False,
                                    default_flow_style=False)
        license_header = INDEX_LICENSE_HEADER_TEMPLATE.format(
            year=time.gmtime().tm_year)
        index_path.write_text(f'{license_header}\n{index_yaml}',
                              encoding='utf-8',
                              newline='')
        logging.info('Wrote toolchain index %s:', index_path)
        Console().print(Syntax(index_path.read_bytes().decode('utf-8'),
                               'yaml'))

    def run(self, clear: bool = False, force_overwrite: bool = False) -> None:
        """Execute the full inspect-stage-read-pack pipeline.

        On a successful pack, the transient `self._staged_xcode` working
        tree is removed; on failure it is left in place so the user can
        inspect what went wrong.

        Args:
            clear: If True, delete every entry under `self._out_dir` at the
                start of the run so the build produces output into a
                guaranteed-clean directory.
            force_overwrite: If True, skips the early "already published"
                check so an existing index for this toolchain is rebuilt and
                replaced instead of refused. See `_precheck_publishable`.

        Raises:
            FileNotFoundError: If a YAML entry refers to a path that is not
                present under `self._staged_xcode`.
            RuntimeError: If Chromium's `mac_sdk.gni` is missing either of the
                expected assignments, if no released Universal Xcode bundles
                the upstream SDK build (or more than one does), if the resolved
                release lists no SHA-1 checksum, if no Xcode `.xip` can be
                downloaded and SHA-1-verified, if `xip` does not expand an
                `Xcode.app`, if the selected Xcode build does not match the
                resolved one, if `xcode-select` does not point at an
                Xcode.app, if `xcodebuild` does not report all of Xcode /
                Build version / SDKVersion / ProductBuildVersion, if
                `xcrun --find metal` does not resolve to a `Metal.xctoolchain`,
                or if a published index already exists for this toolchain and
                `force_overwrite` was not set.
            urllib.error.HTTPError: If a gitiles fetch fails (typically a
                bad `--chromium-tag`).
            subprocess.CalledProcessError: If any invoked tool
                (`xcode-select`, `xcodebuild`, `xcrun`, `xip`, `cp`, `rsync`)
                exits non-zero.
        """
        if clear:
            logging.info('Clearing contents of %s', self._out_dir)
            shutil.rmtree(self._out_dir, ignore_errors=True)
        self._out_dir.mkdir(parents=True, exist_ok=True)
        self._load_upstream_mac_sdk_info()
        if not force_overwrite:
            self._precheck_publishable()
        self._resolve_xcode_release()
        self._install_xcode()
        self._locate_xcode_app()
        self._load_local_versions()
        self._stage_xcode()
        self._add_metal_toolchain()
        self._read_entries()
        output = self._archive_path
        self._build_archive()
        logging.info('Wrote %s (%d bytes)', output, output.stat().st_size)
        logging.info('Removing staged Xcode at %s', self._staged_xcode)
        shutil.rmtree(self._staged_xcode)
        self._write_index()


def main(argv: list[str] | None = None) -> int:
    """Parse CLI arguments and pack the toolchain."""
    parser = argparse.ArgumentParser(description=(
        'Builds a .tar.gz archive of a hermetic Xcode toolchain.'))
    parser.add_argument(
        '--out-dir',
        required=True,
        type=Path,
        help='Directory used to build the toolchain and produce the resulting '
        '.tar.gz archive with the toolchain in it.')
    parser.add_argument(
        '--chromium-tag',
        required=True,
        help='Chromium release tag (e.g. 150.0.7841.1) used to fetch details'
        'about the pinned SDK version to be archived.')
    parser.add_argument(
        '--clear',
        action='store_true',
        help='Makes sure the output directory is empty before building.')
    parser.add_argument(
        '--force-overwrite',
        action='store_true',
        help='Overwrite the published index for this toolchain instead of '
        'refusing when one already exists.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Log every archive member at DEBUG level.')
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format='%(message)s')

    ToolchainBuilder(args.chromium_tag,
                     args.out_dir).run(clear=args.clear,
                                       force_overwrite=args.force_overwrite)
    return 0


if __name__ == '__main__':
    sys.exit(main())
