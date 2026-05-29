#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download and install bucket-hosted archives declared in `EXTRA_DEPS`.

This is a small, general-purpose installer for archives Brave publishes to its
own download bucket but that can't be expressed as a gclient `gcs` dependency
(e.g. they live behind plain HTTP rather than a GCS bucket). The first consumer
is the prebuilt Rust WASM toolchain, but nothing here is specific to it.

`EXTRA_DEPS` mirrors the shape of a gclient `gcs` dependency, keyed by the
checkout-relative destination path. Each object lists an `object_name`, its
`sha256sum`, and a `condition`. For a given entry, the single object whose
condition matches is downloaded, verified against its hash, and extracted into
the destination. The archive is expected to lay its members out relative to the
destination root, so it can be extracted straight on top of it.

`condition` strings (both dep-level and per-object) are resolved with
depot_tools by loading the checkout's `.gclient` config and the owning
solution's DEPS, so every variable resolves exactly as it would during a real
`gclient sync` — this script never maps or hardcodes variable values itself.
"""

from __future__ import annotations

import argparse
import logging
import sys
import tarfile
import tempfile
from pathlib import Path

# `src/` directory (this script lives at src/brave/tools/cr/toolchains/).
_SRC_DIR = Path(__file__).resolve().parents[4]

# Importing script and depot_tools explicitly, so we can call this script from
# the terminal without needing to set up the PYTHONPATH. depot_tools is
# inserted at the front so its `third_party` package (which provides
# `third_party.schema`, needed by gclient_eval) is not shadowed by the
# `third_party` package in the vpython virtualenv.
sys.path.append(str(Path(__file__).resolve().parents[3] / 'script'))
sys.path.insert(0, str(_SRC_DIR / 'third_party' / 'depot_tools'))

import deps  # pylint: disable=wrong-import-position
import gclient  # pylint: disable=wrong-import-position,import-error
import gclient_eval  # pylint: disable=wrong-import-position,import-error
import gclient_paths  # pylint: disable=wrong-import-position,import-error
import gclient_utils  # pylint: disable=wrong-import-position,import-error

EXTRA_DEPS = {
    'src/third_party/rust-toolchain': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/rust-toolchain-aux/',
        'condition': 'not rust_force_head_revision',
        'objects': [
            {
                'object_name': 'linux-x64-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-2-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': 'bcfc7b7b273bcce1570f835727f53c53be0e319a87cd7da9ac7fe2e03752bee4',
                'condition': 'host_os == "linux"',
            },
            {
                'object_name': 'mac-arm64-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-2-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': '2fd9e14a5eb879c893fdfca25fb752f3599f1753cbca7e6da404c60721bb9060',
                'condition': 'host_os == "mac" and host_cpu == "arm64"',
            },
            {
                'object_name': 'mac-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-2-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': 'fe14b9a46c3cd16d9da2b0a8dab0a9a23f791af9b3a8be617d83915bbf961bc1',
                'condition': 'host_os == "mac" and host_cpu == "x64"',
            },
            {
                'object_name': 'win-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-2-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': '1529acb4b61ac48b8ec3c4fe6178d67261b2b5ba64ceca42c2f5abcbe3f60cf1',
                'condition': 'host_os == "win"',
            },
        ],
    },
}


def _select_object(objects: list[dict],
                   variables: dict[str, object]) -> dict | None:
    """Return the single object whose condition matches the resolved variables.

    Returns None if no object matches, and raises if more than one does, since
    an entry is expected to have exactly one matching object (e.g. one per
    host platform).
    """
    matches = [
        obj for obj in objects
        if gclient_eval.EvaluateCondition(obj['condition'], variables)
    ]
    if not matches:
        return None
    if len(matches) > 1:
        raise RuntimeError('Multiple objects match the resolved variables: ' +
                           ', '.join(obj['object_name'] for obj in matches))
    return matches[0]


def _extract(archive_path: Path, dest_dir: Path) -> None:
    """Overlay the archive onto the destination directory.

    The archive is expected to lay its members out relative to the destination
    root, so it is extracted straight into `dest_dir`, overwriting any files it
    matches and leaving everything else in the directory untouched.

    Extraction uses tarfile's `data` filter (PEP 706) so a malformed archive
    cannot escape `dest_dir` via absolute paths, `..` traversal, or links
    pointing outside it. This is defense-in-depth: the archive's bytes are
    already pinned by the `sha256sum` verified before extraction.
    """
    with tarfile.open(archive_path, mode='r:*') as tar:
        tar.extractall(path=dest_dir, filter='data')


def _hash_file_name(object_name: str) -> str:
    """Return the sidecar hash filename for *object_name*.

    Mirrors gclient's `gcs` scheme rather than hardcoding a name: the hash file
    is `.{file_prefix}_hash`, where file_prefix is the object name with '/' and
    '.' replaced by '_' (see GcsDependency.gcs_file_name / file_prefix /
    hash_file in depot_tools' gclient.py). Deriving it per object means the
    sidecar is keyed to the exact object, exactly like gclient's.
    """
    file_prefix = object_name.replace('/', '_').replace('.', '_')
    return f'.{file_prefix}_hash'


def _installed_hash(dest_dir: Path, object_name: str) -> str:
    """Return the sha256 recorded by the last successful install, or ''."""
    hash_file = dest_dir / _hash_file_name(object_name)
    if hash_file.is_file():
        return hash_file.read_bytes().decode('utf-8').strip()
    return ''


def _record_hash(dest_dir: Path, object_name: str, sha256sum: str) -> None:
    """Persist the installed sha256 for subsequent freshness checks."""
    hash_file = dest_dir / _hash_file_name(object_name)
    hash_file.write_text(f'{sha256sum}\n', encoding='utf-8', newline='')


class ExtraDepsInstaller:
    """Installs `EXTRA_DEPS` entries with gclient-resolved conditions.

    Holds the loaded `.gclient` context for a run so each solution's variables
    are resolved once (via depot_tools) and shared across the entries processed.
    """

    def __init__(self, client: gclient.GClient) -> None:
        self._client = client
        # Fully-resolved DEPS vars, cached lazily per solution name.
        self._vars_by_solution: dict[str, dict[str, object]] = {}

    @classmethod
    def from_checkout(cls) -> ExtraDepsInstaller:
        """Build an installer from the checkout's `.gclient` config.

        Anchored at `_SRC_DIR` rather than the process cwd so it resolves
        regardless of where the hook is invoked from.
        """
        parser = gclient.OptionParser()
        options, _ = parser.parse_args([])
        root = gclient_paths.FindGclientRoot(str(_SRC_DIR),
                                             options.config_filename)
        if root is None:
            raise RuntimeError(
                f'Could not find a .gclient root from {_SRC_DIR}')
        client = gclient.GClient(root, options)
        client.SetConfig(
            gclient_utils.FileRead(Path(root) / options.config_filename))
        return cls(client)

    def _solution_vars(self, name: str) -> dict[str, object]:
        """Return (and cache) the fully-resolved DEPS variables for a solution.

        This is the exact dict gclient builds while processing that solution's
        DEPS during a sync: the DEPS-declared `vars` overlaid with `.gclient`
        custom_vars, plus the built-in vars (`host_os`, `host_cpu`,
        `checkout_*`, ...). Letting depot_tools load and merge everything means
        this script never has to know which variables exist or how to compute
        their values — conditions resolve exactly as for a real `gclient sync`.
        """
        if name not in self._vars_by_solution:
            solution = next(
                (d for d in self._client.dependencies if d.name == name), None)
            if solution is None:
                names = [d.name for d in self._client.dependencies]
                raise RuntimeError(
                    f'No gclient solution named {name!r} (have: {names})')
            # Populates the solution's `_vars` from its DEPS file; `get_vars`
            # then layers parent, built-in, and custom_vars on top.
            solution.ParseDepsFile()
            self._vars_by_solution[name] = solution.get_vars()
        return self._vars_by_solution[name]

    def install(self, path: str, spec: dict) -> None:
        """Download and install the matching object for one `EXTRA_DEPS` entry.
        """
        variables = self._solution_vars(path.split('/', 1)[0])

        condition = spec.get('condition')
        if condition and not gclient_eval.EvaluateCondition(
                condition, variables):
            logging.info('Skipping %s: condition %r is false', path, condition)
            return

        obj = _select_object(spec['objects'], variables)
        if obj is None:
            logging.info('No matching object for %s on this host', path)
            return

        dest_dir = _SRC_DIR.parent / path
        object_name = obj['object_name']
        sha256sum = obj['sha256sum']
        if _installed_hash(dest_dir, object_name) == sha256sum:
            logging.info('%s already up to date (%s)', path, object_name)
            return

        url = spec['bucket'] + object_name
        with tempfile.TemporaryDirectory() as tmp_dir:
            archive_path = Path(tmp_dir) / object_name
            with archive_path.open('wb') as archive_file:
                deps.DownloadUrl(url, archive_file)
            deps.VerifySHA256(str(archive_path), sha256sum, url)
            _extract(archive_path, dest_dir)
        _record_hash(dest_dir, object_name, sha256sum)
        logging.info('Installed %s into %s', object_name, dest_dir)


def main() -> int:
    logging.basicConfig(level=logging.INFO, format='%(message)s')

    parser = argparse.ArgumentParser(
        description='Download and install the bucket-hosted archive for the '
        'given EXTRA_DEPS entry.')
    parser.add_argument(
        'dep',
        choices=sorted(EXTRA_DEPS),
        metavar='DEP_PATH',
        help='Path key in EXTRA_DEPS identifying the entry to install '
        '(e.g. src/third_party/rust-toolchain).')
    args = parser.parse_args()

    ExtraDepsInstaller.from_checkout().install(args.dep, EXTRA_DEPS[args.dep])
    return 0


if __name__ == '__main__':
    sys.exit(main())
