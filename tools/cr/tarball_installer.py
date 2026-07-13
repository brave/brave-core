# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`TarballInstaller`: fetch, verify, and extract one `EXTRA_DEPS` object.

Stdlib-only: it downloads over HTTP, verifies the sha256, extracts the archive,
and writes the sidecar files that record what is deployed. Also runnable as a
CLI to deploy a single-object `EXTRA_DEPS` entry into the checkout.
"""

from __future__ import annotations

import argparse
from collections.abc import Callable
from dataclasses import dataclass
import hashlib
import json
import shutil
import sys
import tarfile
import tempfile
import time
import zipfile
from pathlib import Path
from urllib.error import URLError
from urllib.parse import urlsplit
from urllib.request import urlopen

from extra_deps import EXTRA_DEPS, is_deployed, sidecar_path

# The workspace root (parent of `src`): this file is at
# `<root>/src/brave/tools/cr/tarball_installer.py`.
_WORKSPACE_ROOT = Path(__file__).resolve().parents[4]

# Only single-object entries can be deployed here: picking an object out of a
# multi-object entry needs host-condition resolution this installer does not do.
_INSTALLABLE = sorted(path for path, spec in EXTRA_DEPS.items()
                      if len(spec['objects']) == 1)


@dataclass(frozen=True)
class TarballInstaller:
    """Installs one resolved `EXTRA_DEPS` object into its destination.

    Handles the mechanics for a single object: sha256 verification, extraction
    (wiping the destination first when it owns it), and writing the sidecar
    files `extra_deps` reads back to tell a deployed tree from a stale one:

      .{prefix}_hash                the object's sha256
      .{prefix}_content_names       JSON list of the archive's members

    where `prefix` is the object name with `/` and `.` mapped to `_`.
    """

    # The checkout directory the archive is extracted into.
    dest_dir: Path
    # The full URL the archive is downloaded from.
    url: str
    # The bucket object name; drives sidecar naming and diagnostics.
    object_name: str
    # The expected sha256 of the archive.
    sha256sum: str
    # True when the dep owns dest_dir (wiped before extract); False when it
    # overlays an existing upstream tree (extracted on top).
    owns_dest: bool

    @classmethod
    def for_dep(cls, root: Path, path: str, spec: dict) -> TarballInstaller:
        """Build an installer for the single object of `spec` under `root`.

        `root` is the workspace root (the parent of `src`) and `path` the
        checkout-relative destination. Raises `ValueError` for a multi-object
        entry: picking one of its objects needs host-condition resolution this
        installer does not do.
        """
        objects = spec['objects']
        if len(objects) != 1:
            raise ValueError(
                f'{path}: only single-object entries can be installed here, '
                f'but this one has {len(objects)}')
        obj = objects[0]
        return cls(dest_dir=root / path,
                   url=spec['bucket'] + obj['object_name'],
                   object_name=obj['object_name'],
                   sha256sum=obj['sha256sum'],
                   owns_dest=not obj.get('overlayed_on'))

    def is_installed(self) -> bool:
        """True when the `_hash` sidecar already records `sha256sum`."""
        return is_deployed(self.dest_dir, self.object_name, self.sha256sum)

    def install(self,
                download: Callable[[str, object], None] | None = None) -> bool:
        """Fetch and extract the object, writing the sidecars on success.

        `download(url, file_obj)` writes the archive bytes into `file_obj`;
        it defaults to `_download` (a stdlib urllib fetch with live progress)
        and can be overridden in tests to avoid the network. Returns False when
        the sidecar already records `sha256sum` and nothing needed to be done.
        """
        if self.is_installed():
            return False
        download = download or self._download
        with tempfile.TemporaryDirectory() as tmp_dir:
            archive_path = Path(tmp_dir) / self.object_name
            with archive_path.open('wb') as archive_file:
                download(self.url, archive_file)
            self._verify(archive_path)
            if self.owns_dest and self.dest_dir.exists():
                # Drop the previous extraction so nothing stale lingers;
                # overlays deliberately keep the upstream tree they sit on.
                shutil.rmtree(self.dest_dir)
            member_names = self._extract(archive_path)
        self._write_sidecars(member_names)
        return True

    def _verify(self, archive_path: Path) -> None:
        """Raise `ValueError` unless `archive_path` hashes to `sha256sum`."""
        digest = hashlib.sha256()
        # Not using mmap to make this script more tolerable to a larger range
        # of Python versions, as this script is called by `launcher.py` without
        # a guarantee of vpython being available.
        with archive_path.open('rb') as archive_file:
            for block in iter(lambda: archive_file.read(1 << 20), b''):
                digest.update(block)
        actual = digest.hexdigest()
        if actual.lower() != self.sha256sum.lower():
            raise ValueError(f'SHA-256 mismatch for {self.url}\n'
                             f'  expected: {self.sha256sum.lower()}\n'
                             f'  actual:   {actual}')

    def _download(self, url: str, output_file) -> None:
        """Fetch `url` into `output_file` with a live progress line.

        A stdlib urllib fetch that retries (3 tries, doubling backoff, no retry
        on 403/404), plus a carriage-return progress line on stderr in the
        gsutil style `[<done>/<total>] <pct>%`. Refuses any non-`https` URL:
        the sha256 check catches tampering, but the transport must be encrypted.
        """
        if urlsplit(url).scheme != 'https':
            raise ValueError(f'refusing to fetch over a non-https URL: {url}')
        num_retries = 3
        retry_wait_s = 5  # Doubled at each retry.
        while True:
            try:
                output_file.seek(0)
                output_file.truncate()
                with urlopen(url) as response:
                    total = int(response.headers.get('Content-Length') or 0)
                    done = 0
                    last_pct = -1
                    while chunk := response.read(64 * 1024):
                        output_file.write(chunk)
                        done += len(chunk)
                        pct = int(done * 100 / total) if total else -1
                        if pct != last_pct:
                            last_pct = pct
                            self._emit_progress(done, total)
                    if total and done != total:
                        raise URLError(f'only got {done} of {total} bytes')
                    self._emit_progress(done, total, done_flag=True)
                return
            except URLError as error:
                sys.stderr.write(f'\n{error}\n')
                # `code` is an HTTPError-only attribute (a URLError subclass).
                if num_retries == 0 or getattr(error, 'code',
                                               None) in (403, 404):
                    raise
                num_retries -= 1
                sys.stderr.write(f'Retrying in {retry_wait_s} s ...\n')
                time.sleep(retry_wait_s)
                retry_wait_s *= 2

    def _emit_progress(self, done: int, total: int, done_flag=False) -> None:
        """Rewrite the stderr progress line for `done`/`total` bytes."""
        if total:
            line = (f'{self._human(done)}/{self._human(total)}] '
                    f'{int(done * 100 / total):>3}%')
        else:
            line = f'{self._human(done)}]'
        tail = ' Done\n' if done_flag else ''
        sys.stderr.write(f'\r{self.object_name} [{line}{tail}')
        sys.stderr.flush()

    @staticmethod
    def _human(num: float) -> str:
        """A base-1024 size string (`26.7 MiB`), matching gsutil's format."""
        for unit in ('B', 'KiB', 'MiB', 'GiB', 'TiB'):
            if num < 1024 or unit == 'TiB':
                return f'{round(num, 1):g} {unit}'
            num /= 1024
        return f'{num:g} TiB'  # unreachable; keeps the type checker happy

    def _extract(self, archive_path: Path) -> list[str]:
        """Extract the archive (tar or zip) into `dest_dir`.

        Returns the archive's member names (`ZipFile.namelist()` /
        `tar.getnames()`), recorded in the `_content_names` sidecar.
        """
        if zipfile.is_zipfile(archive_path):
            with zipfile.ZipFile(archive_path) as archive:
                names = archive.namelist()
                archive.extractall(path=self.dest_dir)
                return names
        with tarfile.open(archive_path, mode='r:*') as tar:
            names = tar.getnames()
            tar.extractall(path=self.dest_dir, filter='data')
            return names

    def _write_sidecars(self, member_names: list[str]) -> None:
        """Write the sidecar set, each with a `.stamp` tail."""
        contents = {
            '_hash': self.sha256sum,
            '_content_names': json.dumps(member_names),
        }
        for suffix, content in contents.items():
            sidecar_path(self.dest_dir, self.object_name,
                         suffix).write_text(f'{content}\n',
                                            encoding='utf-8',
                                            newline='')


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description='Deploy a single-object EXTRA_DEPS entry into this '
        'checkout.')
    parser.add_argument('dep',
                        choices=_INSTALLABLE,
                        metavar='DEP_PATH',
                        help='The EXTRA_DEPS path key to deploy.')
    args = parser.parse_args(argv)
    TarballInstaller.for_dep(_WORKSPACE_ROOT, args.dep,
                             EXTRA_DEPS[args.dep]).install()
    return 0


if __name__ == '__main__':
    sys.exit(main())
