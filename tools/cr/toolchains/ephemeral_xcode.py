# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Resolve, deploy, and select the Xcode that ships a target macOS SDK.
"""

from __future__ import annotations

import dataclasses
import hashlib
import json
import logging
import re
import shutil
import sys
import tempfile
import urllib.error
import urllib.request
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path
from typing import Any

from rich.progress import (BarColumn, DownloadColumn, Progress,
                           TaskProgressColumn, TextColumn, TimeRemainingColumn,
                           TransferSpeedColumn)

# `cherry_picks` is a sibling module; add this directory to the path so the
# import resolves whether this module is imported by a sibling build script or
# run on its own.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from cherry_picks import _check_call  # pylint: disable=wrong-import-position

# Per-request timeout for the small HTTP fetches (gitiles raw files and the
# xcodereleases.com JSON).
HTTP_FETCH_TIMEOUT_SECS = 30

# Read timeout for the (multi-gigabyte) Xcode `.xip` download. Generous because
# it bounds individual socket reads against a large, slow transfer.
XIP_DOWNLOAD_TIMEOUT_SECS = 600

# Chunk size used to stream the Xcode `.xip` download and to hash it.
XIP_IO_CHUNK_BYTES = 1024 * 1024

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


@dataclasses.dataclass(frozen=True)
class MacSdkInfo:
    """macOS SDK version triple reported by `xcodebuild -version -sdk macosx`.
    """

    # macOS SDK version, e.g. `26.5`.
    sdk_version: str

    # Product build number for that SDK, e.g. `25F70`.
    product_build_version: str

    @staticmethod
    def from_gni(mac_sdk_gni_text: str) -> MacSdkInfo:
        """Parse the macOS SDK pin out of `build/config/mac/mac_sdk.gni`.

        `mac_sdk_gni_text` is the verbatim file contents. gn auto-formats these
        assignments as `key = "value"`, so a simple regex recovers each value.

        Raises:
            RuntimeError: if either expected assignment is missing.
        """

        def gn_value(key: str) -> str:
            match = re.search(rf'{key} = "([^"]+)"', mac_sdk_gni_text)
            if not match:
                raise RuntimeError(f'{key} not found in mac_sdk.gni')
            return match.group(1)

        return MacSdkInfo(
            sdk_version=gn_value('mac_sdk_official_version'),
            product_build_version=gn_value('mac_sdk_official_build_version'))


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
    # field, used to verify the download.
    sha1: str


class EphemeralXcode:
    """Deploys and selects the Xcode that ships a target macOS SDK.

    This class takes care of resolving the exact Xcode release that bundles the
    requested SDK. Installs are done under `/Applications/xcode_<build>.app`,
    with the build number being used as a key. If an exising install is already
    found in place, we just select the pre-existing install.
    """

    def __init__(self):
        """Initialize an unresolved manager; call `deploy()` to populate it."""
        # Path to the active local Xcode.app, populated by `_locate_app()`
        # from `xcode-select -p`.
        self._app: Path | None = None

        # The Xcode release resolved from `xcodereleases.com` for the requested
        # macOS SDK build, populated by `_resolve_release()` and deployed by
        # `_install()`.
        self._release: XcodeRelease | None = None

    @property
    def app(self) -> Path:
        """The active `Xcode.app` resolved by the last `deploy()`."""
        if self._app is None:
            raise RuntimeError('deploy() must run before accessing app')
        return self._app

    @property
    def release(self) -> XcodeRelease:
        """The Xcode release resolved by the last `deploy()`."""
        if self._release is None:
            raise RuntimeError('deploy() must run before accessing release')
        return self._release

    @contextmanager
    def deploy(self, mac_sdk_info: MacSdkInfo) -> Iterator[EphemeralXcode]:
        """Resolve, install, and select the Xcode for an SDK pin, then reset.

        A context manager. Resolves the released Xcode that ships
        `mac_sdk_info`, deploys it, selects it as the active Xcode and verifies
        the active Xcode/SDK versions match, then yields with `release` and
        `app` populated and the resolved Xcode active (so `xcodebuild`/`xcrun`
        resolve against it). At the end, the context always reverts back to
        `sudo xcode-select --reset`.

        Args:
            mac_sdk_info: The macOS SDK version/build the deployed Xcode must
                ship (typically Chromium's `mac_sdk.gni` pin).

        Yields:
            This `EphemeralXcode`, with `app`/`release` populated.
        """
        self._resolve_release(mac_sdk_info)
        app_path = self._install()
        with self._select(app_path):
            self._locate_app()
            self._verify_versions(mac_sdk_info)
            yield self

    def _resolve_release(self, mac_sdk_info: MacSdkInfo) -> None:
        """Map a macOS SDK build to a released Xcode `.xip`.

        Queries `xcodereleases.com` and looks for the *released*  Xcode entries
        whose bundled macOS SDK build matches `mac_sdk_info`'s build. Results
        are also filtered down to the released Universal archive, which is what
        we use in our infra. Stores the result on `self._release`.
        """
        target_build = mac_sdk_info.product_build_version
        data = _fetch_xcode_releases()

        candidates: list[XcodeRelease] = []
        for entry in data:
            # The catalog lists other products (Swift Playgrounds, Command Line
            # Tools, ...) alongside Xcode; we only care about Xcode entries.
            if entry.get('name') != 'Xcode':
                continue
            version = entry['version']
            # Only final releases: the `release` object carries a truthy
            # `release` key. Betas (`{'beta': N}`), release candidates
            # (`{'rc': N}`), GM seeds, etc. all lack it and are skipped.
            if not (version.get('release') or {}).get('release'):
                continue
            macos_sdks = entry['sdks']['macOS']
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
                             sha1=entry['checksums']['sha1']))

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
        self._release = chosen[0]
        logging.info('Resolved Xcode %s (build %s) -> %s (sha1 %s)',
                     self._release.version, self._release.build,
                     self._release.xip_filename, self._release.sha1)

    def _install(self) -> Path:
        """Ensure the resolved Xcode is expanded on disk, installing if needed.

        Reuses an Xcode previously expanded on this node, keyed by build number,
        under `XCODE_APPS_DIR` as `xcode_<build>.app`. If it is not already
        present, the `.xip` is downloaded and expanded into place. Returns the
        path to the expanded `Xcode.app`.
        """
        assert self._release is not None
        app_name = f'xcode_{self._release.build.lower()}.app'
        app_path = XCODE_APPS_DIR / app_name
        if app_path.exists():
            logging.info('Reusing Xcode already expanded at %s', app_path)
        else:
            self._download_and_expand(app_path)
        return app_path

    def _download_and_expand(self, app_path: Path) -> None:
        """Download the resolved `.xip` and expand it to *app_path*.

        Fetches the archive into an auto-cleaned temp dir, verifies it against
        its expected SHA-1 provided by `xcodereleases.com`, then expands the
        `xip` archive, and finally the resulting `Xcode.app` is moved into
        `app_path`. The expansion happens on the same volume as `app_path` so
        the final move is an instantaneous rename rather than a multi-gigabyte
        copy.
        """
        assert self._release is not None

        # Expand into a sibling of the destination (same volume) so the final
        # `Xcode.app` -> `xcode_<build>.app` move is a rename, not a copy.
        expand_dir = app_path.with_name(f'.{app_path.stem}.expand')
        if expand_dir.exists():
            shutil.rmtree(expand_dir)
        expand_dir.mkdir(parents=True)
        try:
            # The multi-gigabyte `.xip` is only read during expansion, so it
            # lives in a temp dir that cleans itself up rather than lingering in
            # any output tree.
            with tempfile.TemporaryDirectory(prefix='xcode-xip-') as tmp_dir:
                xip_path = Path(tmp_dir) / self._release.xip_filename
                self._download_xip(xip_path)
                # `unxip` and `xip --expand` both write `Xcode.app` into the
                # current directory and take no output-path flag, hence the
                # dedicated `cwd`. `unxip` is dramatically faster, but is not
                # installed by default, so prefer it when present and fall back
                # to Apple's `xip` otherwise.
                if shutil.which('unxip'):
                    _check_call('unxip', str(xip_path), cwd=expand_dir)
                else:
                    logging.warning(
                        '`unxip` not found on PATH; falling back to the slower '
                        '`xip --expand`. Installing `unxip` (`brew install '
                        'unxip`) is strongly preferred for much faster Xcode '
                        'expansion.')
                    _check_call('xip',
                                '--expand',
                                str(xip_path),
                                cwd=expand_dir)
            expanded_app = expand_dir / 'Xcode.app'
            if not expanded_app.exists():
                raise RuntimeError(
                    f'xip did not produce Xcode.app under {expand_dir}')
            logging.info('Installing expanded Xcode to %s', app_path)
            shutil.move(expanded_app, app_path)
        finally:
            shutil.rmtree(expand_dir, ignore_errors=True)

    def _download_xip(self, dest: Path) -> None:
        """Download the resolved, SHA-1-verified `.xip` into *dest*.

        Tries the published filename first, then a variant with the `_Universal`
        segment stripped (DevOps sometimes strips that portion from the name).
        Each download is checked against the `checksums.sha1` value
        xcodereleases.com reports for the release. An unverifiable archive is
        never left on disk.
        """
        assert self._release is not None
        expected_sha1 = self._release.sha1
        filename = self._release.xip_filename
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

    @contextmanager
    def _select(self, app_path: Path) -> Iterator[None]:
        """Make *app_path* the active Xcode for the duration of the context.

        Switches the developer dir with `sudo xcode-select -s`, and runs
        `xcodebuild -runFirstLaunch` so the freshly expanded Xcode finishes its
        one-time setup. On exit always reverts back to the default installation
        with `sudo xcode-select --reset`, so the machine is never left pointing
        at this ephemeral Xcode.
        """
        _check_call('sudo', '/usr/bin/xcode-select', '-s', str(app_path))
        _check_call('sudo', '/usr/bin/xcodebuild', '-license', 'accept')
        _check_call('sudo', '/usr/bin/xcodebuild', '-runFirstLaunch')

        # This is to avoid issues caused by mixed usage of different Xcode
        # versions on one machine.
        _check_call('xcrun', 'simctl', 'list')
        try:
            yield
        finally:
            _check_call('sudo', '/usr/bin/xcode-select', '--reset')

    def _locate_app(self) -> None:
        """Resolve the local Xcode.app from the system developer dir.

        `xcode-select -p` prints the active developer directory, typically
        `/Applications/Xcode.app/Contents/Developer`. Walk two parents up
        to recover the .app bundle itself, then sanity-check the `.app`
        suffix so a misconfigured xcode-select (pointing at e.g.
        CommandLineTools) fails loud and early rather than producing a
        useless archive.
        """
        developer_dir = _check_call('xcode-select', '-p',
                                    capture_output=True).stdout.strip()
        app = Path(developer_dir).parent.parent
        if app.suffix != '.app':
            raise RuntimeError(
                f'xcode-select -p returned {developer_dir!r}; expected a '
                f'path inside an Xcode.app bundle (derived app={app}).')
        self._app = app
        logging.info('Detected Xcode at %s', self._app)

    def _verify_versions(self, mac_sdk_info: MacSdkInfo) -> None:
        """Probe `xcodebuild` and confirm the active Xcode and SDK match.

        Runs `xcodebuild -version -sdk macosx`, whose output carries both the
        Xcode-level lines (`Xcode <ver>` / `Build version <build>`) and the SDK-
        level lines (`SDKVersion:` / `ProductBuildVersion:`). All four values
        are load-bearing: the Xcode build is checked against the resolved
        release and the SDK pair against `mac_sdk_info`, so a wrong active
        Xcode/SDK fails the build. Missing any one of them is a hard failure
        rather than a silent fallback.
        """
        output = _check_call('xcodebuild',
                             '-version',
                             '-sdk',
                             'macosx',
                             capture_output=True).stdout
        sdk_version = re.search(r'^SDKVersion: (.+)$', output, re.MULTILINE)
        sdk_build = re.search(r'^ProductBuildVersion: (.+)$', output,
                              re.MULTILINE)

        output = _check_call('xcodebuild', '-version',
                             capture_output=True).stdout
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

        # `deploy()` resolves the release before probing, so it must be set.
        assert self._release is not None

        # `_select()` should have made the Xcode we resolved active. If the
        # selected Xcode build doesn't match the one we are expected to be
        # using to build the SDK, we must fail the run.
        if local_xcode_info.build != self._release.build:
            raise RuntimeError(
                f'Active Xcode build {local_xcode_info.build} does not '
                f'match the resolved build {self._release.build}; '
                'xcode-select may be pointing at the wrong Xcode.')

        # The deployed Xcode must ship the exact macOS SDK Chromium, otherwise
        # the build would be using an incorrect SDK.
        if local_mac_sdk_info != mac_sdk_info:
            raise RuntimeError(
                'Active macOS SDK '
                f'{local_mac_sdk_info.sdk_version} '
                f'(build {local_mac_sdk_info.product_build_version}) '
                'does not match the upstream-pinned '
                f'{mac_sdk_info.sdk_version} '
                f'(build {mac_sdk_info.product_build_version}).')

        logging.info('Local Xcode: %s (build %s)', local_xcode_info.version,
                     local_xcode_info.build)
        logging.info('Local macOS SDK: %s (build %s)',
                     local_mac_sdk_info.sdk_version,
                     local_mac_sdk_info.product_build_version)
