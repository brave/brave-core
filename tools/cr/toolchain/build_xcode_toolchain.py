#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build a hermetic, reproducible Xcode toolchain archive for Chromium.

Keep this as a *standalone* script that can be invoked directly on a macOS CI
node. PyYAML is the only non-stdlib import; if it is not already importable it
is installed via `pip --user` on the first run, so a fresh CI image needs no
preconfiguration beyond `python3` and `pip`.

To run directly from GitHub:

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchain/build_xcode_toolchain.py \
    | python3 - \
        --out-dir=./out/ \
        --chromium-tag=150.0.7841.1
```

The script is intentionally self-contained: a CI invocation only needs a
Chromium tag and an output directory. It:

  1. Detects the local Xcode.app via `xcode-select -p`.
  2. Clones it into a staging tree under `--out-dir` using BSD `cp -ac`
     (APFS clonefile, near-instant on the same volume).
  3. Downloads the on-demand Metal toolchain via
     `xcodebuild -downloadComponent metalToolchain` and rsyncs its
     artifacts into the staged `XcodeDefault.xctoolchain`.
  4. Fetches `build/xcode_binaries.yaml` from Chromium at `--chromium-tag`
     via gitiles.
  5. Packs the matching paths from the staged tree into a gzip-compressed
     tar with all per-entry metadata zeroed (mtime, uid, gid, uname, gname)
     and symlink targets normalized via `os.path.normpath`.

The output archive is written under `--out-dir` as
`xcode-hermetic-toolchain-<xcode-version>-<xcode-build>-<sdk-version>-
            <sdk-build>-for-upstream-<sdk-version>-<sdk-build>.tar.gz`,
where the leading pair is what `xcodebuild -version` reports for the local
Xcode app on the build host, and the `for-upstream` pair is the SDK
(`mac_sdk_official_version` / `mac_sdk_official_build_version`) that
Chromium pins in `build/config/mac/mac_sdk.gni` at `--chromium-tag`.

The full download URL is logged at the end of a successful run:
`<PACKAGE_DOWNLOAD_URL_BASE>/<archive-filename>`.
"""

from __future__ import annotations

import argparse
import base64
import dataclasses
import gzip
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

import yaml

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

# Per-request timeout for gitiles fetches. Unfortunately gitlies can flake a
# bit in new tags. Unfortunately gitlies some times takes some time to answer
# requests about new tags.
GITILES_FETCH_TIMEOUT_SECS = 30
GITILES_FETCH_MAX_ATTEMPTS = 3
GITILES_FETCH_RETRY_DELAY_SECS = 2

# The base url used by brave to download the toolchain package. This is used in
# this script only to produce a log line with resulting URL.
PACKAGE_DOWNLOAD_URL_BASE = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws/'
    'xcode-hermetic-toolchain/')


def _check_call(*command,
                cwd=None,
                capture_stdout: bool = False) -> str | None:
    """Run *command* as a subprocess, logging the invocation.

    Logs the full command string at INFO level before executing it.  Stderr
    is inherited from the parent process so subprocess output streams
    directly to the terminal.

    Args:
        *command: The program and its arguments (passed as positional args,
            not as a list).
        cwd: Optional working directory for the subprocess.  Defaults to the
            caller's current working directory when `None`.
        capture_stdout: When `True`, capture the child's stdout and return it
            as a UTF-8 decoded string.  When `False` (default), stdout is
            inherited from the parent and the function returns `None`.

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
                    url, timeout=GITILES_FETCH_TIMEOUT_SECS) as response:
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


def _get_assigned_value(contents: str, lookup: str) -> str | None:
    """Extract the value assigned to *lookup* in a gn/Python-style key file.

    Scans *contents* line by line for `<lookup> = <value>`, discarding any
    `#`-style trailing comment and stripping single/double quotes from the
    value. Returns the value verbatim (minus quotes) or `None` if the key
    is not assigned.
    """
    for line in contents.splitlines():
        # Discard any trailing comment on the line.
        line = line.split('#', 1)[0].strip()
        if lookup not in line or '=' not in line:
            continue
        key, value = line.split('=', 1)
        if key.strip() == lookup:
            return value.strip().replace('"', '').replace("'", '')
    return None


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


class ToolchainBuilder:
    """A builder for the XCode hermetic toolchain archive.

    This builder does a few things:

    1. **Versioning** The builder determines which version of Xcode is being
       used locally, and which version Chromium was using for the given tag,
       and uses this information to construct a unique name for the output
       archive
    2. **Stage** (`_stage_xcode` + `_add_metal_toolchain`): Clones the
       detected Xcode.app into `self.staged_xcode` using BSD `cp -ac`
       (APFS clonefile), then downloads the on-demand Metal toolchain
       via `xcodebuild -downloadComponent metalToolchain` and rsyncs its
       artifacts into the staged `XcodeDefault.xctoolchain`.
    3. **Read** (`_fetch_pkg_def` / `_read_entries`): Fetches
       `xcode_binaries.yaml` from gitiles with the tag provided and stores the
       parsed `(kind, relpath)` tuples on `self.entries`.
    4. **Pack** (`_build_archive` / `_pack`): Streams every entry from
       `self.staged_xcode` into the output `.tar.gz` with
       `_normalize_tar_entry` applied so the bytes are reproducible
       across hosts.
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
        self.chromium_tag: str = chromium_tag

        # Absolute path of the directory the output archive is written into.
        self.out_dir: Path = out_dir.expanduser().resolve()

        # Path to the local Xcode.app, populated by `_locate_xcode_app()`
        # from `xcode-select -p`.
        self.xcode_app: Path | None = None

        # Transient working tree where Xcode is cloned and augmented with
        # the Metal toolchain before `_build_archive` packs it. Created
        # by `_stage_xcode()` and removed by `run()` after a successful
        # archive build.
        self.staged_xcode: Path = self.out_dir / 'staged_xcode'

        # Xcode version and build, to be used in the archive's filename.
        self.local_xcode_info: XcodeInfo | None = None

        # The macOS SDK version and build to be used in the archive.
        self.local_mac_sdk_info: MacSdkInfo | None = None

        # The macOS SDK version and build that Chromium is using. This value is
        # merely encoded in the archive name, but this allows brockit to check
        # for its existence.
        self.upstream_mac_sdk_info: MacSdkInfo | None = None

        # Parsed `xcode_binaries.yaml` entries.
        self.entries: list[tuple[str, str]] = []

    @property
    def _archive_path(self) -> Path:
        """Path of the output archive.

        The file name is constructed in a way that we may be able through
        through brockit to check if the toolchain is already archived in our
        infra, and if Brave is using it, just based on the CL culprit that
        updated the SDK in upstream.
        """
        if (self.local_xcode_info is None
                or self.upstream_mac_sdk_info is None):
            raise RuntimeError(
                '_load_local_versions() and _load_upstream_mac_sdk_info() '
                'must run before _archive_path')
        return self.out_dir / (
            'xcode-hermetic-toolchain'
            f'-{self.local_xcode_info.version}'
            f'-{self.local_xcode_info.build}'
            f'-{self.local_mac_sdk_info.sdk_version}'
            f'-{self.local_mac_sdk_info.product_build_version}'
            f'-for-upstream-{self.upstream_mac_sdk_info.sdk_version}'
            f'-{self.upstream_mac_sdk_info.product_build_version}'
            '.tar.gz')

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
        self.xcode_app = app
        logging.info('Detected Xcode at %s', self.xcode_app)

    def _stage_xcode(self) -> None:
        """Clone the detected Xcode.app into `self.staged_xcode`.

        Uses BSD `cp -ac`: `-a` is archive mode (preserve everything),
        `-c` enables APFS `clonefile()` so the copy is near-instant when
        source and destination share a volume. The trailing `/` on the
        source path means the *contents* of the .app are copied into the
        destination, so the staged tree begins at `Contents/...` —
        matching the layout `_pack` and Chromium's `xcode_binaries.yaml`
        expect.
        """
        assert self.xcode_app is not None
        if self.staged_xcode.exists():
            logging.info('Removing existing stage at %s', self.staged_xcode)
            shutil.rmtree(self.staged_xcode)
        self.staged_xcode.mkdir(parents=True)
        _check_call('cp', '-ac', f'{self.xcode_app}/', str(self.staged_xcode))

    def _add_metal_toolchain(self) -> None:
        """Download the Metal toolchain and graft it into the staged Xcode.

        Recent Xcode releases stopped shipping the Metal compiler inside
        the .app bundle; it is an on-demand component installed via
        `xcodebuild -downloadComponent metalToolchain` into a system-wide
        cache. Once present, `xcrun --find metal` resolves the `metal`
        binary; we truncate that path back to the enclosing
        `Metal.xctoolchain` directory and rsync the relevant subtrees
        into the staged `XcodeDefault.xctoolchain` so the packager picks
        them up.
        """
        _check_call('xcodebuild', '-downloadComponent', 'metalToolchain')
        metal_bin = _check_call('xcrun',
                                '--find',
                                'metal',
                                capture_stdout=True).strip()
        match = re.search(r'^(.*/Metal\.xctoolchain)/', metal_bin)
        if not match:
            raise RuntimeError(
                'Could not derive Metal.xctoolchain from `xcrun --find '
                f'metal` output: {metal_bin!r}')
        metal_dir = Path(match.group(1))
        dest = (self.staged_xcode /
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

        Runs `xcodebuild -version -sdk macosx`, whose output carries both
        the Xcode-level lines (`Xcode <ver>` / `Build version <build>`)
        and the SDK-level lines (`SDKVersion:` / `ProductBuildVersion:`).
        All four fields are load-bearing: the Xcode pair stamps the
        archive filename and the SDK pair is logged for drift comparison
        against `self.upstream_mac_sdk_info`. Missing any one of them is a
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
        self.local_xcode_info = XcodeInfo(
            version=xcode_version.group(1).strip(),
            build=xcode_build.group(1).strip())
        self.local_mac_sdk_info = MacSdkInfo(
            sdk_version=sdk_version.group(1).strip(),
            product_build_version=sdk_build.group(1).strip())
        logging.info('Local Xcode: %s (build %s)',
                     self.local_xcode_info.version,
                     self.local_xcode_info.build)
        logging.info('Local macOS SDK: %s (build %s)',
                     self.local_mac_sdk_info.sdk_version,
                     self.local_mac_sdk_info.product_build_version)

    def _load_upstream_mac_sdk_info(self) -> None:
        """Fetch the macOS SDK used in the Chromium tag provided.

        Reads `build/config/mac/mac_sdk.gni` from gitiles and pulls the
        `mac_sdk_official_version` and `mac_sdk_official_build_version`
        from these sources, storing them in their corresponding fields.
        """
        url = MAC_SDK_GNI_URL_TEMPLATE.format(tag=self.chromium_tag)
        text = _fetch_gitiles_raw(url)
        sdk_version = _get_assigned_value(text, 'mac_sdk_official_version')
        product_build_version = _get_assigned_value(
            text, 'mac_sdk_official_build_version')
        if not sdk_version or not product_build_version:
            raise RuntimeError(
                'mac_sdk_official_version / mac_sdk_official_build_version '
                f'not found in {url}')
        self.upstream_mac_sdk_info = MacSdkInfo(
            sdk_version=sdk_version,
            product_build_version=product_build_version)
        logging.info('Upstream macOS SDK version: %s (build %s)',
                     self.upstream_mac_sdk_info.sdk_version,
                     self.upstream_mac_sdk_info.product_build_version)

    def _fetch_pkg_def(self) -> str:
        """Fetch `xcode_binaries.yaml` from gitiles."""
        return _fetch_gitiles_raw(
            PKG_DEF_URL_TEMPLATE.format(tag=self.chromium_tag))

    def _read_entries(self) -> None:
        """Load the `data:` list from `xcode_binaries.yaml` in document order.

        Each entry is stored as `(kind, relpath)` on `self.entries`, where
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
        self.entries = entries

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
        """
        for kind, relpath in self.entries:
            source = self.staged_xcode / relpath
            # `is_symlink()` catches dangling symlinks that `exists()` skips.
            if not source.exists() and not source.is_symlink():
                raise FileNotFoundError(
                    f'pkg_def {kind}: {relpath!r} not found under '
                    f'{self.staged_xcode}')
            logging.debug('+ %s', relpath)
            tar.add(source, arcname=relpath, filter=_normalize_tar_entry)

    def _build_archive(self) -> None:
        """Write the deterministic `.tar.gz` under `self.out_dir`.

        The gzip outer layer is opened with `mtime=0` and an empty `filename`
        so the gzipped bytes on disk are reproducible across runs (the tar
        payload is already reproducible by virtue of per-entry normalization).
        PAX format is selected so long member names and UTF-8 metadata are
        encoded portably.
        """
        output = self._archive_path
        with output.open('wb') as raw_fp, \
             gzip.GzipFile(filename='', mode='wb', fileobj=raw_fp,
                           mtime=0) as gz, \
             tarfile.open(fileobj=gz, mode='w',
                          format=tarfile.PAX_FORMAT) as tar:
            self._pack(tar)

    def run(self) -> None:
        """Execute the full inspect-stage-read-pack pipeline.

        On a successful pack, the transient `self.staged_xcode` working
        tree is removed; on failure it is left in place so the user can
        inspect what went wrong.

        Raises:
            FileNotFoundError: If a YAML entry refers to a path that is not
                present under `self.staged_xcode`.
            RuntimeError: If `xcode-select` does not point at an Xcode.app,
                if `xcodebuild` does not report all of Xcode / Build
                version / SDKVersion / ProductBuildVersion, if Chromium's
                `mac_sdk.gni` is missing either of the expected
                assignments, or if `xcrun --find metal` does not resolve
                to a `Metal.xctoolchain`.
            urllib.error.HTTPError: If a gitiles fetch fails (typically a
                bad `--chromium-tag`).
            subprocess.CalledProcessError: If any invoked tool
                (`xcode-select`, `xcodebuild`, `xcrun`, `cp`, `rsync`)
                exits non-zero.
        """
        self.out_dir.mkdir(parents=True, exist_ok=True)
        self._locate_xcode_app()
        self._load_local_versions()
        self._load_upstream_mac_sdk_info()
        self._stage_xcode()
        self._add_metal_toolchain()
        self._read_entries()
        output = self._archive_path
        logging.info('Packing %d entries from %s into %s', len(self.entries),
                     self.staged_xcode, output)
        self._build_archive()
        logging.info('Wrote %s (%d bytes)', output, output.stat().st_size)
        logging.info('Download URL: %s%s', PACKAGE_DOWNLOAD_URL_BASE,
                     output.name)
        logging.info('Removing staged Xcode at %s', self.staged_xcode)
        shutil.rmtree(self.staged_xcode)


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
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Log every archive member at DEBUG level.')
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format='%(message)s')

    ToolchainBuilder(args.chromium_tag, args.out_dir).run()
    return 0


if __name__ == '__main__':
    sys.exit(main())
