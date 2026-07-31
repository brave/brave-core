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
"""Build a hermetic, reproducible Windows toolchain archive for Chromium.

Windows-only: it installs the Visual Studio + Windows SDK that Chromium pins in
`build/vs_toolchain.py` at a given `--chromium-ref` onto this machine, then
packages a hermetic MSVC + Windows SDK toolchain from that install via
depot_tools' `win_toolchain/package_from_installed.py`.

To run this script:

```sh
vpython3 tools/cr/toolchains/build_windows_toolchain.py \
    --out-dir=./out/ \
    --chromium-ref=150.0.7841.1
```

What it does in summary:

  1. Reads the `SDK_VERSION` / `TOOLCHAIN_HASH` pins (plus the packaged Visual
     Studio version) that Chromium sets in `build/vs_toolchain.py` at
     `--chromium-ref`, via gitiles.
  2. Installs that exact Visual Studio (via the `aka.ms/vs/<major>/stable`
     bootstrapper) and the exact Windows SDK build Chromium expects (scraped
     from Microsoft's SDK downloads page, since the VS workload alone may pull
     a different SDK build than the one pinned).
  3. Verifies the installed SDK actually matches the upstream pin (both the
     `SDK_VERSION` directory and, best-effort, the exact
     `sdk_version_in_comment` installer build) before packaging -- Microsoft's
     own installers can silently stealth-update a release without bumping its
     version.
  4. Packages the install with depot_tools'
     `win_toolchain/package_from_installed.py`, which writes a content-hash-
     named `<toolchain_hash>.zip`.
  5. Writes a sibling YAML index next to the archive, named after the upstream
     `TOOLCHAIN_HASH` pin -- so it is queryable by whatever
     `build/vs_toolchain.py` currently pins -- recording the bucket URL, the
     archive SHA-256, and the toolchain provenance. Publishing is refused if an
     index already exists for that upstream hash.
  6. With `--upload`, publishes the archive and its sibling index to
     `TOOLCHAIN_BUCKET`. This is the same internal bucket + prefix the
     `windows-hermetic-toolchain-build` Jenkins job's native `s3Upload`
     pipeline step currently uploads `out/toolchain/*` to.
"""

from __future__ import annotations

import argparse
import ctypes
import dataclasses
import json
import logging
import os
import re
import shutil
import stat
import subprocess
import sys
import urllib.request
from pathlib import Path

# This is necessary because these scripts are used by brockit too.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from cherry_picks import _check_call  # pylint: disable=wrong-import-position
import gitiles  # pylint: disable=wrong-import-position
import toolchain_publish  # pylint: disable=wrong-import-position
from upload import sha256_file  # pylint: disable=wrong-import-position

TOOLCHAIN_BUCKET = 'brave-build-deps-internal'
TOOLCHAIN_BUCKET_PREFIX = 'windows-hermetic-toolchain'
PACKAGE_DOWNLOAD_URL_BASE = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws/'
    'windows-hermetic-toolchain/')

# A single-quoted string value as it appears in `build/vs_toolchain.py`, e.g.
# the `'10.0.26100.0'` in `SDK_VERSION = '10.0.26100.0'`.
QUOTED_VALUE = r"'([^']+)'"

# The full SDK build written in the descriptive comment near the top of
# `build/vs_toolchain.py`, e.g. the `10.0.26100.7705` in
# `# VS 2026 17.13.4 with 10.0.26100.7705 SDK ...`. Distinct from the
# `SDK_VERSION` pin, whose final component is zeroed (`10.0.26100.0`).
SDK_VERSION_IN_COMMENT = r'# VS .* with ([\d.]+) SDK'

# The packaged Visual Studio entry in `build/vs_toolchain.py`'s
# `MSVS_VERSIONS`, e.g. `('2026', '18.0')`. The first entry is the one Chromium
# packages; upstream flags it with the trailing comment we anchor on. Group 1
# is the marketing year (`2026`), group 2 the version (`18.0`).
PACKAGED_VS_VERSION = (r"\('(\d+)', '([\d.]+)'\),\s*"
                       r"# The VS version in our packaged toolchain")

VS_BOOTSTRAPPER_URL_TEMPLATE = (
    'https://aka.ms/vs/{major}/stable/vs_professional.exe')
WINDOWS_SDK_DOWNLOADS_URL = (
    'https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads')

# Read timeout for an installer (bootstrapper) download. The bootstrappers
# themselves are small; the bulk of each install is pulled by the installer.
INSTALLER_DOWNLOAD_TIMEOUT_SECS = 300

# Workloads and components the bootstrapper installs.
VS_COMPONENTS = [
    'Microsoft.VisualStudio.Workload.NativeDesktop',
    'Microsoft.VisualStudio.Component.VC.ATL',
    'Microsoft.VisualStudio.Component.VC.ATLMFC',
    'Microsoft.VisualStudio.Component.VC.ATL.ARM64',
    'Microsoft.VisualStudio.Component.VC.Tools.ARM64',
    'Microsoft.VisualStudio.Component.VC.MFC.ARM64',
]

# Standard install location of `vswhere.exe`, used to read the installed VS
# instance recorded in the toolchain index.
VSWHERE_PATH = Path(
    r'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe')

DEPOT_TOOLS_PYTHON3 = 'vpython3.bat'
PACKAGE_FROM_INSTALLED_RELPATH = (Path('win_toolchain') /
                                  'package_from_installed.py')

def _resolve_windows_sdk_installer_url(build: str) -> str:
    """Resolve the standalone installer URL for an exact Windows SDK *build*.

    Scrapes `WINDOWS_SDK_DOWNLOADS_URL`, where each release is a table row whose
    first cell carries the build in parentheses (e.g. `(10.0.26100.7705)`) and
    whose second cell links the installer:

        <td><strong>Windows SDK for Windows 11 (10.0.26100.7705)</strong> ...
        <td><a href="https://go.microsoft.com/fwlink/?linkid=2349110" ...>
            Installer</a> <br> <a href="...">ISO</a></td>

    Returns the `go.microsoft.com/fwlink` URL of the first `Installer` anchor
    that follows the matching build. Raises `RuntimeError` if *build* is not
    listed on the page.
    """
    with urllib.request.urlopen(
            WINDOWS_SDK_DOWNLOADS_URL,
            timeout=gitiles.HTTP_FETCH_TIMEOUT_SECS) as response:
        html = response.read().decode('utf-8')

    # From the parenthesized build, the next anchor labeled `Installer` is the
    # one for that row (the intervening `Release notes` anchor is skipped since
    # its link text differs).
    match = re.search(
        re.escape(f'({build})') + r'.*?href="([^"]+)"[^>]*>\s*Installer', html,
        re.DOTALL)
    if not match:
        raise RuntimeError(
            f'No Windows SDK installer for build {build} found on '
            f'{WINDOWS_SDK_DOWNLOADS_URL}')

    url = match.group(1)
    # A few entries use protocol-relative `//go.microsoft.com/...` links.
    return f'https:{url}' if url.startswith('//') else url


def _rmtree(path: Path) -> None:
    """Recursively remove *path*, clearing the read-only bit first.
    """
    if not path.exists():
        return

    def _make_writable_and_retry(func, target, _exc_info):
        os.chmod(target, stat.S_IWRITE)
        func(target)

    shutil.rmtree(path, onerror=_make_writable_and_retry)


def _download_to_file(url: str, dest: Path) -> None:
    """Stream the response body from *url* into *dest*.
    """
    logging.info('Downloading %s -> %s', url, dest)
    with urllib.request.urlopen(
            url, timeout=INSTALLER_DOWNLOAD_TIMEOUT_SECS) as response, \
         dest.open('wb') as out:
        shutil.copyfileobj(response, out)


@dataclasses.dataclass(frozen=True)
class WinSdkInfo:
    """Windows toolchain identity pinned by Chromium in `build/vs_toolchain.py`.
    """

    # Windows SDK version, e.g. `10.0.26100.0` (the `SDK_VERSION` pin).
    sdk_version: str

    # depot_tools hermetic toolchain hash (the `TOOLCHAIN_HASH` pin).
    toolchain_hash: str

    # Full SDK build as written in the descriptive comment at the top of
    # `build/vs_toolchain.py`, e.g. `10.0.26100.7705`. Unlike `sdk_version`
    # (whose final component is zeroed), this carries the precise SDK build.
    sdk_version_in_comment: str

    # Marketing year of the packaged Visual Studio, e.g. `2026` (first key of
    # `MSVS_VERSIONS` in `build/vs_toolchain.py`).
    vs_year: str

    # Version of the packaged Visual Studio, e.g. `18.0`. Its major component
    # selects the `aka.ms/vs/<major>/...` bootstrapper to install.
    vs_version: str

    @property
    def vs_major_version(self) -> str:
        """Major component of `vs_version`, e.g. `18` -- the `aka.ms` selector.
        """
        return self.vs_version.split('.')[0]

    @classmethod
    def from_vs_toolchain_py(cls, text: str) -> WinSdkInfo:
        """Parse the pins out of `build/vs_toolchain.py`'s source *text*.

        Raises:
            RuntimeError: if any expected assignment is missing.
        """

        def value(pattern: str, description: str) -> str:
            match = re.search(pattern, text)
            if not match:
                raise RuntimeError(
                    f'Could not find {description} in build/vs_toolchain.py.')
            return match.group(1)

        packaged_vs = re.search(PACKAGED_VS_VERSION, text)
        if not packaged_vs:
            raise RuntimeError(
                'Could not find the packaged Visual Studio version in '
                'build/vs_toolchain.py (the MSVS_VERSIONS entry tagged '
                '"# The VS version in our packaged toolchain").')
        return cls(sdk_version=value(f'SDK_VERSION = {QUOTED_VALUE}',
                                     'SDK_VERSION'),
                   toolchain_hash=value(f'TOOLCHAIN_HASH = {QUOTED_VALUE}',
                                        'TOOLCHAIN_HASH'),
                   sdk_version_in_comment=value(SDK_VERSION_IN_COMMENT,
                                                'the full SDK build comment'),
                   vs_year=packaged_vs.group(1),
                   vs_version=packaged_vs.group(2))


def toolchain_index_name(sdk_info: WinSdkInfo) -> str:
    """`<upstream_toolchain_hash>.yaml`, the sibling index for an SDK pin.

    Naming the index after the upstream `TOOLCHAIN_HASH` pin so we can discover
    a particular toolchain that we have produced ourselves from the one
    expected in upstream.
    """
    return f'{sdk_info.toolchain_hash}.yaml'


def fetch_published_index(sdk_info: WinSdkInfo) -> dict:
    """Download and parse the published toolchain index for an SDK pin.

    Mirrors `build_xcode_toolchain.fetch_published_index`: resolves the sibling
    YAML index for *sdk_info* on the public download bucket and returns it
    verbatim (the mapping `ToolchainBuilder._write_index` published).

    Raises:
        RuntimeError: if the index cannot be fetched.
    """
    index_url = PACKAGE_DOWNLOAD_URL_BASE + toolchain_index_name(sdk_info)
    return toolchain_publish.fetch_index(index_url,
                                         'hermetic Windows toolchain')


class ToolchainBuilder:
    """A builder for the hermetic Windows (MSVC + SDK) toolchain archive.

    This builder does a few things:

    1. **Versioning** (`_load_upstream_sdk_info`): reads the Windows SDK /
       toolchain pins Chromium sets in `build/vs_toolchain.py` for the given
       tag.
    2. **Install** (`_install_visual_studio` + `_install_windows_sdk`): installs
       the pinned Visual Studio via the `aka.ms` bootstrapper, then the exact
       Windows SDK build Chromium expects (the VS workload alone may pull a
       different SDK build). Unlike the Xcode builder there is no reuse: this
       assumes a disposable node that starts without either installed.
    3. **Package** (`_build_archive`): runs depot_tools'
       `win_toolchain/package_from_installed.py` to produce a content-hash-named
       `<toolchain_hash>.zip`.
    4. **Index** (`_precheck_publishable` / `_write_index`): refuses (early) to
       clobber an already-published toolchain, then writes Brave's sibling
       YAML index, named after the upstream `TOOLCHAIN_HASH` pin so it is
       queryable by what `build/vs_toolchain.py` pins.
    """

    def __init__(self, chromium_tag: str, out_dir: Path):
        """Resolve the output directory and remember the requested tag.

        Args:
            chromium_tag: Chromium release tag (e.g. `150.0.7841.1`) used to
                fetch `build/vs_toolchain.py` from gitiles.
            out_dir: Directory where the resulting archive (and any transient
                staging tree) are written.
        """
        # The Chromium tag being used for the gitiles download.
        self._chromium_tag: str = chromium_tag

        # Absolute path of the directory the output archive is written into.
        self._out_dir: Path = out_dir.expanduser().resolve()

        # The Windows SDK / toolchain pins Chromium uses, populated by
        # `_load_upstream_sdk_info()`.
        self._upstream_sdk_info: WinSdkInfo | None = None

    @property
    def _index_path(self) -> Path:
        """Path of Brave's sibling YAML index. See `toolchain_index_name`."""
        if self._upstream_sdk_info is None:
            raise RuntimeError(
                '_load_upstream_sdk_info() must run before _index_path')
        return self._out_dir / toolchain_index_name(self._upstream_sdk_info)

    def _depot_tools_dir(self) -> Path:
        """Resolve the depot_tools root from `gclient` on PATH.

        depot_tools is expected to already be deployed and on PATH by the
        caller (the recipe's `depot_tools` module ensures this before invoking
        this script); this script does not bootstrap it.
        """
        gclient = shutil.which('gclient')
        if gclient is None:
            raise RuntimeError(
                'depot_tools not found on PATH: `gclient` is not resolvable. '
                'This script expects depot_tools to already be deployed '
                'before it runs.')
        return Path(gclient).resolve().parent

    def _load_upstream_sdk_info(self) -> None:
        """Read the Windows SDK / toolchain pins from `build/vs_toolchain.py`.

        Fetches the file from gitiles at `self._chromium_tag` and stores the
        parsed pins on `self._upstream_sdk_info`.
        """
        text = gitiles.fetch_chromium_file(self._chromium_tag,
                                           'build/vs_toolchain.py')
        self._upstream_sdk_info = WinSdkInfo.from_vs_toolchain_py(text)
        logging.info(
            'Upstream Windows toolchain: VS %s (%s), SDK %s (full build %s), '
            'toolchain hash %s', self._upstream_sdk_info.vs_year,
            self._upstream_sdk_info.vs_version,
            self._upstream_sdk_info.sdk_version,
            self._upstream_sdk_info.sdk_version_in_comment,
            self._upstream_sdk_info.toolchain_hash)

    def _install_visual_studio(self) -> None:
        """Download the VS Professional bootstrapper and install it silently.

        The bootstrapper major version is taken from the packaged VS version in
        `build/vs_toolchain.py` (`MSVS_VERSIONS`' first entry), so e.g. `18.0`
        installs from `https://aka.ms/vs/18/stable/vs_professional.exe`. The
        `stable` channel link always resolves to the latest build of that major
        version.
        """
        assert self._upstream_sdk_info is not None
        url = VS_BOOTSTRAPPER_URL_TEMPLATE.format(
            major=self._upstream_sdk_info.vs_major_version)

        bootstrapper = self._out_dir / 'vs_professional.exe'
        logging.info('Installing Visual Studio %s (%s) from %s',
                     self._upstream_sdk_info.vs_year,
                     self._upstream_sdk_info.vs_version, url)
        _download_to_file(url, bootstrapper)

        # `--passive --wait --norestart` run a non-interactive install (progress
        # UI shown, no prompts) and block until it finishes;
        # `--includeRecommended` pulls each workload's recommended components
        # (e.g. the Windows SDK behind NativeDesktop); `--locale en-US` keeps
        # the install language stable; each `--add` selects a workload/component
        # from `VS_COMPONENTS`.
        add_flags = [arg for c in VS_COMPONENTS for arg in ('--add', c)]
        _check_call(str(bootstrapper), '--passive', '--wait', '--norestart',
                    '--locale', 'en-US', '--includeRecommended', *add_flags)

    def _install_windows_sdk(self) -> None:
        """Download and install the exact Windows SDK build Chromium pins.

        `_install_visual_studio` installs whichever Windows SDK its workload
        recommends, which may not be the precise build Chromium pins. This
        resolves the standalone installer for `sdk_version_in_comment` from
        Microsoft's downloads page and installs that exact build alongside it.
        """
        assert self._upstream_sdk_info is not None
        build = self._upstream_sdk_info.sdk_version_in_comment
        url = _resolve_windows_sdk_installer_url(build)

        installer = self._out_dir / 'winsdksetup.exe'
        logging.info('Installing Windows SDK %s from %s', build, url)
        _download_to_file(url, installer)

        # `/features +` installs all SDK features; `/quiet /norestart` run a
        # non-interactive install without rebooting; `/ceip off` opts out of
        # telemetry; `/log` captures a log for diagnosing headless failures.
        log_path = self._out_dir / 'winsdksetup.log'
        _check_call(str(installer), '/features', '+', '/quiet', '/norestart',
                    '/ceip', 'off', '/log', str(log_path))

    @staticmethod
    def _installed_sdk_root() -> Path:
        """Resolve the installed Windows SDK root from the registry.

        Mirrors how both `build/vs_toolchain.py`'s
        `SetEnvironmentAndGetSDKDir` and depot_tools'
        `package_from_installed.py` resolve the SDK root.
        """
        output = _check_call(
            'reg',
            'query',
            r'HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots',
            '/v',
            'KitsRoot10',
            capture_output=True).stdout
        match = re.search(r'KitsRoot10\s+REG_SZ\s+(.+)', output)
        if not match:
            raise RuntimeError(
                'Could not resolve the installed Windows SDK root from the '
                r'registry (KitsRoot10 under HKLM\SOFTWARE\Microsoft\Windows '
                r'Kits\Installed Roots).')
        return Path(match.group(1).strip().rstrip('\\'))

    @staticmethod
    def _installed_sdk_builds() -> list[str]:
        """Every exact Windows SDK build recorded in Add/Remove Programs.

        `build/vs_toolchain.py` itself notes there is no general, reliable way
        to read an installed SDK directory's exact patch build back off disk:
        the `Include`/`Lib` folder is always named after the coarser,
        last-component-zeroed `SDK_VERSION`, regardless of which patch build
        was installed into it. This instead reads the precise build from each
        installer's own Add/Remove Programs entry (its `DisplayName` embeds
        the full version, e.g. "Windows Software Development Kit - Windows
        10.0.28000.2270"), checking both the 32-bit and native registry views
        since installers can register in either.
        """
        uninstall_keys = (
            r'HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion'
            r'\Uninstall',
            r'HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall',
        )
        builds = []
        for uninstall_key in uninstall_keys:
            try:
                subkeys = _check_call('reg',
                                      'query',
                                      uninstall_key,
                                      capture_output=True).stdout
            except subprocess.CalledProcessError:
                continue
            for line in subkeys.splitlines():
                subkey = line.strip()
                if not subkey.startswith('HKEY_'):
                    continue
                entry = _check_call('reg',
                                    'query',
                                    subkey,
                                    '/v',
                                    'DisplayName',
                                    capture_output=True,
                                    check=False).stdout
                match = re.search(
                    r'Windows Software Development Kit.*?'
                    r'(\d+\.\d+\.\d+\.\d+)', entry)
                if match:
                    builds.append(match.group(1))
        return builds

    def _verify_installed_sdk(self) -> None:
        """Verify the pinned Windows SDK build is actually installed.

        `_install_windows_sdk` downloads and runs the installer for
        `sdk_version_in_comment`, but Microsoft's own installers can silently
        stealth-update a release without bumping its version (see this
        module's docstring), and `package_from_installed.py` filters its file
        list by a bare substring match on `SDK_VERSION` -- so a wrong or
        missing install produces a silently incomplete package rather than a
        hard failure. This catches that before packaging (and, in turn,
        before publishing).

        Raises:
            RuntimeError: if the `SDK_VERSION` directory is missing, or if the
                pinned `sdk_version_in_comment` build isn't among the
                installed SDK builds recorded in Add/Remove Programs.
        """
        assert self._upstream_sdk_info is not None
        sdk_info = self._upstream_sdk_info

        include_dir = (self._installed_sdk_root() / 'Include' /
                       sdk_info.sdk_version)
        if not include_dir.is_dir():
            raise RuntimeError(
                f'Expected Windows SDK {sdk_info.sdk_version} headers at '
                f'{include_dir}, but that directory does not exist. '
                'package_from_installed.py would silently produce an '
                'incomplete Windows Kits payload for this SDK version.')

        installed_builds = self._installed_sdk_builds()
        logging.info('Installed Windows SDK builds found: %s', installed_builds
                     or '(none)')
        if sdk_info.sdk_version_in_comment not in installed_builds:
            raise RuntimeError(
                f'Pinned Windows SDK build '
                f'{sdk_info.sdk_version_in_comment!r} from '
                'build/vs_toolchain.py was not found among the installed SDK '
                f'builds recorded in Add/Remove Programs: {installed_builds}. '
                'Refusing to package a toolchain that may not match what was '
                'requested.')

    def _build_archive(self) -> Path:
        """Package the installed toolchain with depot_tools; return the `.zip`.
        """
        assert self._upstream_sdk_info is not None
        depot_tools = self._depot_tools_dir()
        python3 = depot_tools / DEPOT_TOOLS_PYTHON3
        package_script = depot_tools / PACKAGE_FROM_INSTALLED_RELPATH

        # Always package into a clean staging dir: `package_from_installed.py`
        # writes its `<hash>.zip` into the working directory, and a stale zip
        # would break the single-archive expectation below.
        toolchain_dir = self._out_dir / 'toolchain'
        _rmtree(toolchain_dir)
        toolchain_dir.mkdir(parents=True)

        _check_call(str(python3),
                    str(package_script),
                    self._upstream_sdk_info.vs_year,
                    '-w',
                    self._upstream_sdk_info.sdk_version,
                    '--allow_multiple_vs_installs',
                    cwd=toolchain_dir)

        zips = list(toolchain_dir.glob('*.zip'))
        if len(zips) != 1:
            raise RuntimeError(
                f'expected exactly one toolchain .zip in {toolchain_dir}, '
                f'found {len(zips)}: {zips}')
        logging.info('Packaged toolchain archive: %s', zips[0])
        return zips[0]

    @staticmethod
    def _installed_vs_instance() -> dict:
        """Return the active `vswhere -latest` instance as a dict.
        """
        return json.loads(
            _check_call(str(VSWHERE_PATH),
                        '-latest',
                        '-format',
                        'json',
                        capture_output=True).stdout)[0]

    def _precheck_publishable(self) -> None:
        """Fail fast if an index for this upstream toolchain hash is published.
        """
        assert self._upstream_sdk_info is not None
        index_url = PACKAGE_DOWNLOAD_URL_BASE + self._index_path.name
        if toolchain_publish.remote_url_exists(index_url):
            raise RuntimeError(
                f'{index_url} already exists; a toolchain for upstream hash '
                f'{self._upstream_sdk_info.toolchain_hash} is already '
                'published.')

    def _write_index(self, archive: Path) -> None:
        """Write Brave's sibling `<toolchain_hash>.yaml` bucket index.
        """
        assert self._upstream_sdk_info is not None
        sdk_info = self._upstream_sdk_info
        vswhere_instance = self._installed_vs_instance()
        installed_vs_version = vswhere_instance['catalog'][
            'productDisplayVersion']

        index = {
            'url': PACKAGE_DOWNLOAD_URL_BASE + archive.name,
            'sha256sum': sha256_file(archive),
            'size_bytes': archive.stat().st_size,
            'hash': archive.stem,
            'sdk_version': sdk_info.sdk_version,
            'sdk_version_in_comment': sdk_info.sdk_version_in_comment,
            'upstream_toolchain_hash': sdk_info.toolchain_hash,
            'upstream_vs_version': sdk_info.vs_version,
            'installed_vs_version': installed_vs_version,
            'installed_vs_display_name': vswhere_instance['displayName'],
            'chromium_tag': self._chromium_tag,
        }
        index['brave_core_commit'] = toolchain_publish.brave_core_commit()

        toolchain_publish.write_index_file(self._index_path, index)

    def _upload(self, archive: Path) -> None:
        """Upload the archive and its sibling index to the internal bucket.
        """
        toolchain_publish.upload_files(TOOLCHAIN_BUCKET,
                                       TOOLCHAIN_BUCKET_PREFIX,
                                       (archive, self._index_path))

    def run(self, clear: bool = False, upload: bool = False) -> None:
        """Execute the full inspect-install-pack-index-upload pipeline.

        Args:
            clear: If True, delete every entry under `self._out_dir` at the
                start of the run so the build produces output into a
                guaranteed-clean directory.
            upload: If True, upload the archive and its sibling index to
                `TOOLCHAIN_BUCKET` after building (see `_upload`).

        Raises:
            RuntimeError: If not running elevated, if `depot_tools` is not on
                PATH, if a published index already exists for this toolchain,
                if the installed Windows SDK doesn't match the upstream pin
                (see `_verify_installed_sdk`), or if
                `package_from_installed.py` did not produce exactly one
                `.zip`.
            urllib.error.HTTPError: If a gitiles fetch fails (typically a bad
                `--chromium-tag`).
            subprocess.CalledProcessError: If any invoked tool (the VS/SDK
                installers, `package_from_installed.py`, `vswhere`) exits
                non-zero.
        """
        # Packaging from an installed Visual Studio writes under `Program
        # Files`, so the build must run elevated. `IsUserAnAdmin()` returns 0
        # when the process is not running with administrator rights.
        if ctypes.windll.shell32.IsUserAnAdmin() == 0:
            raise RuntimeError(
                'Please run as Administrator. Write access to Program Files is '
                'required.')

        if clear:
            logging.info('Clearing contents of %s', self._out_dir)
            _rmtree(self._out_dir)
        self._out_dir.mkdir(parents=True, exist_ok=True)

        self._depot_tools_dir()
        self._load_upstream_sdk_info()
        self._precheck_publishable()
        self._install_visual_studio()
        self._install_windows_sdk()
        self._verify_installed_sdk()
        archive = self._build_archive()
        self._write_index(archive)
        if upload:
            self._upload(archive)
        logging.info('Toolchain download URL (once published): %s',
                     PACKAGE_DOWNLOAD_URL_BASE + archive.name)


def main(argv: list[str] | None = None) -> int:
    """Parse CLI arguments and pack the toolchain."""
    parser = argparse.ArgumentParser(description=(
        'Builds an archive of a hermetic Windows (MSVC + SDK) toolchain.'))
    parser.add_argument(
        '--out-dir',
        required=True,
        type=Path,
        help='Directory used to build the toolchain and produce the resulting '
        'archive.')
    parser.add_argument(
        '--chromium-tag',
        required=True,
        help='Chromium release tag (e.g. 150.0.7841.1) used to read the pinned '
        'Windows SDK version / toolchain hash.')
    parser.add_argument(
        '--clear',
        action='store_true',
        help='Makes sure the output directory is empty before building.')
    parser.add_argument(
        '--upload',
        action='store_true',
        help=f'Upload the archive and its sibling index to the internal '
        f'build-deps bucket ({TOOLCHAIN_BUCKET}) after building.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging.')
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format='%(message)s')

    ToolchainBuilder(args.chromium_tag, args.out_dir).run(clear=args.clear,
                                                          upload=args.upload)
    return 0


if __name__ == '__main__':
    sys.exit(main())
