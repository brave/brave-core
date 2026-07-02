#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download and install bucket-hosted archives declared in `EXTRA_DEPS`.

This is a small, general-purpose installer for archives Brave publishes to its
own download bucket, similar to gclient's `gcs` dependency types. This mechanism
was first written for the Rust/WASM toolchain, and to make the process of having
it deployed more generic, and reusable for other cases.

`EXTRA_DEPS` mirrors the shape of a gclient `gcs` dependency, keyed by the
checkout-relative destination path. Each object lists an `object_name`, its
`sha256sum`, an optional per-object `condition`, and optionally `overlayed_on`,
which selects how it is installed:

  * Overlay (with `overlayed_on`) -- extracted on top of the upstream archive it
    names, which is validated against DEPS so we never overlay a rolled base.
  * Owned (without `overlayed_on`) -- fully owns its destination, which is wiped
    and re-extracted each install so no stale extraction lingers.

An entry with a single object may omit its per-object `condition` and gate on
the dep-level `condition` alone.

`bucket` is just an HTTPS prefix `object_name` is appended to: one of Brave's
buckets, or an upstream distribution (e.g. `nodejs.org/dist`) used as one.

`condition` strings (both dep-level and per-object) are resolved with
depot_tools by loading the checkout's `.gclient` config and the owning
solution's DEPS, so every variable resolves exactly as it would during a real
`gclient sync`.
"""

from __future__ import annotations

import argparse
from collections.abc import Mapping
from dataclasses import dataclass
import json
import logging
import shutil
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path

# `src/` directory (this script lives at src/brave/tools/cr/).
_SRC_DIR = Path(__file__).resolve().parents[3]

# Importing script and depot_tools explicitly, so we can call this script from
# the terminal without needing to set up the PYTHONPATH. `depot_tools` is
# inserted at the front so its `third_party` package (which provides
# `third_party.schema`, needed by gclient_eval) is not shadowed by the
# `third_party` package in the vpython virtualenv.
sys.path.append(str(Path(__file__).resolve().parents[2] / 'script'))
sys.path.insert(0, str(_SRC_DIR / 'third_party' / 'depot_tools'))

import deps  # pylint: disable=wrong-import-position
import gclient  # pylint: disable=wrong-import-position,import-error
import gclient_eval  # pylint: disable=wrong-import-position,import-error
import gclient_paths  # pylint: disable=wrong-import-position,import-error
import gclient_utils  # pylint: disable=wrong-import-position,import-error

# This script's own logger, so we don't step on gclient's logger. It is also of
# notice that most logs are DEBUG. No-op runs should not produce any output in
# normal runs, to avoid cluttering the sync output.
_LOG = logging.getLogger('install_extra_deps')

# The extension we use for all sidecar files. Using stamp is deliberate to avoid
# having gclient trying to read our sidecars.
_STAMP = '.stamp'


EXTRA_DEPS = {
    'src/third_party/rust-toolchain': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/rust-toolchain-aux/',
        'condition': 'not rust_force_head_revision',
        'objects': [
            {
                'object_name': 'linux-x64-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': '9e8664e93466c21a3ffedfe4d9e5996a7c967b64575cd2145ab39a6cc3f0ec7f',
                'overlayed_on': 'Linux_x64/rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66.tar.xz',
                'condition': 'host_os == "linux"',
            },
            {
                'object_name': 'mac-arm64-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': 'ed9e927ebde21a8b6f991b598e18050988a295a5f49e4db35ff70ce25e9ffeea',
                'overlayed_on': 'Mac_arm64/rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66.tar.xz',
                'condition': 'host_os == "mac" and host_cpu == "arm64"',
            },
            {
                'object_name': 'mac-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': '6cc780e5baa3f2500633e4ee56dd3116d70c375455052b871ee71e2b90ecad62',
                'overlayed_on': 'Mac/rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66.tar.xz',
                'condition': 'host_os == "mac" and host_cpu == "x64"',
            },
            {
                'object_name': 'win-rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66-1.tar.xz',
                'sha256sum': '869f725b2591892218bcfe70e926e41586ac6bb7921b97b48a2b8cd6468ae569',
                'overlayed_on': 'Win/rust-toolchain-4c4205163abcbd08948b3efab796c543ba1ea687-5-llvmorg-23-init-10931-g20b6ec66.tar.xz',
                'condition': 'host_os == "win"',
            },
        ],
    },
    'src/brave/third_party/node/linux': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/nodejs/',
        'condition': 'host_os == "linux"',
        'objects': [
            {
                'object_name': 'node-v24.17.0-linux-x64.tar.gz',
                'sha256sum': 'e0472427aa791ad80bdc426ff7cc73cdd28ed0f616d1ff9689a23a7f47f1265f',
            },
        ],
    },
    'src/brave/third_party/node/mac': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/nodejs/',
        'condition': 'host_os == "mac" and host_cpu == "x64"',
        'objects': [
            {
                'object_name': 'node-v24.17.0-darwin-x64.tar.gz',
                'sha256sum': '80da552fe037290cb130e9dea590f5eeeb7aa450636f0c89ab41415511c1ec27',
            },
        ],
    },
    'src/brave/third_party/node/mac_arm64': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/nodejs/',
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
        'objects': [
            {
                'object_name': 'node-v24.17.0-darwin-arm64.tar.gz',
                'sha256sum': '4fc3266a3702eebc39cc37661cf4eeceeade307e242ab64e4d7ce7949197e11f',
            },
        ],
    },
    'src/brave/third_party/node/win': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/nodejs/',
        'condition': 'host_os == "win"',
        'objects': [
            {
                'object_name': 'node-v24.17.0-win-x64.zip',
                'sha256sum': 'f2aa33b35b75aca5f3f7b85675a6f6423201053e9381911e64961f3bda2528ab',
            },
        ],
    },
    'src/brave/third_party/ast-grep/ast-grep-linux': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/ast-grep/',
        'condition': 'host_os == "linux"',
        'objects': [
            {
                'object_name': 'ast-grep-0.44.0-linux-x64.tar.gz',
                'sha256sum': 'e5a2d23541d7591fe4ec01190097ca8283a87f4039520271e7cee1ad9998ba5c',
            },
        ],
    },
    'src/brave/third_party/ast-grep/ast-grep-mac': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/ast-grep/',
        'condition': 'host_os == "mac" and host_cpu == "x64"',
        'objects': [
            {
                'object_name': 'ast-grep-0.44.0-mac.tar.gz',
                'sha256sum': 'ad3f3bcae7a91ce070fbe6b934dbfcf35ec9cf004f9c6d7ecb907165669844e2',
            },
        ],
    },
    'src/brave/third_party/ast-grep/ast-grep-mac_arm64': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/ast-grep/',
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
        'objects': [
            {
                'object_name': 'ast-grep-0.44.0-mac-arm64.tar.gz',
                'sha256sum': '5bcb3568679ea6c636298c2b3e1edb44d54a8483f51d5e1ad0778609ef97bea3',
            },
        ],
    },
    'src/brave/third_party/ast-grep/ast-grep-win': {
        'bucket': 'https://brave-build-deps-public.s3.brave.com/ast-grep/',
        'condition': 'host_os == "win"',
        'objects': [
            {
                'object_name': 'ast-grep-0.44.0-win.tar.gz',
                'sha256sum': '1a074eaa2c4ba0e6df2b7a505c45ef26b2605a1360d43c8c2ae4d31624013845',
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
        obj for obj in objects if 'condition' not in obj
        or gclient_eval.EvaluateCondition(obj['condition'], variables)
    ]
    if not matches:
        return None
    if len(matches) > 1:
        raise RuntimeError('Multiple objects match the resolved variables: ' +
                           ', '.join(obj['object_name'] for obj in matches))
    return matches[0]


@dataclass(frozen=True)
class TarballInstaller:
    """Installs one resolved `EXTRA_DEPS` object into its destination.

    Handles the mechanics for a single object: download, sha256 verification,
    extraction (wiping the destination first when it owns it), and the
    gclient-style sidecar bookkeeping used to detect an existing install.

    gclient drops a set of dotfile "sidecars" next to each `gcs` dep's extracted
    tree (see `GcsDependency` in gclient.py and `GcsRoot` in gclient_scm.py),
    keyed by a prefix that is the object name with `/` and `.` replaced by `_`:

      .{prefix}_hash                the object's sha256
      .{prefix}_content_names       JSON list of the archive's members

    We deploy the very same files so our install state mirrors gclient's, but
    append `.stamp` so gclient's own globs (`.*_hash`, `.*_content_names`)
    never read or clobber ours.
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

    def is_installed(self) -> bool:
        """True when the `_hash` sidecar already records `sha256sum`."""
        return self._installed_hash() == self.sha256sum

    def install(self) -> bool:
        """Download and extract the object, writing the sidecars on success.

        Returns False when the sidecar already records `sha256sum` and nothing
        needed to be done.
        """
        if self.is_installed():
            return False
        with tempfile.TemporaryDirectory() as tmp_dir:
            archive_path = Path(tmp_dir) / self.object_name
            with archive_path.open('wb') as archive_file:
                deps.DownloadUrl(self.url, archive_file)
            deps.VerifySHA256(str(archive_path), self.sha256sum, self.url)
            if self.owns_dest and self.dest_dir.exists():
                # Drop the previous extraction so nothing stale lingers;
                # overlays deliberately keep the upstream tree they sit on.
                shutil.rmtree(self.dest_dir)
            member_names = self._extract(archive_path)
        self._write_sidecars(member_names)
        return True

    def _extract(self, archive_path: Path) -> list[str]:
        """Extract the archive (tar or zip) into `dest_dir`.

        Returns the archive's member names, as gclient records them in its
        `.*_content_names` sidecar (`ZipFile.namelist()` / `tar.getnames()`).
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

    def _sidecar(self, suffix: str) -> Path:
        """The `.{prefix}{suffix}.stamp` sidecar path in `dest_dir`."""
        prefix = self.object_name.replace('/', '_').replace('.', '_')
        return self.dest_dir / f'.{prefix}{suffix}{_STAMP}'

    def _installed_hash(self) -> str:
        """The sha256 from the `_hash` sidecar, or '' when absent."""
        hash_file = self._sidecar('_hash')
        if hash_file.is_file():
            return hash_file.read_bytes().decode('utf-8').strip()
        return ''

    def _write_sidecars(self, member_names: list[str]) -> None:
        """Write the gclient-equivalent sidecar set, each with a `.stamp` tail.
        """
        contents = {
            '_hash': self.sha256sum,
            '_content_names': json.dumps(member_names),
        }
        for suffix, content in contents.items():
            self._sidecar(suffix).write_text(f'{content}\n',
                                             encoding='utf-8',
                                             newline='')


class ExtraDepsRunner:
    """Installs `EXTRA_DEPS` entries with gclient-resolved conditions.

    Holds the loaded `.gclient` context for a run so each solution's variables
    are resolved once (via depot_tools) and shared across the entries processed.
    """

    def __init__(self, client: gclient.GClient) -> None:
        self._client = client
        # Parsed DEPS context, cached lazily per solution name. Each entry holds
        # the fully-resolved `vars` and the raw `deps` mapping for the solution.
        self._scope_by_solution: dict[str, dict[str, object]] = {}

    @classmethod
    def from_checkout(cls) -> ExtraDepsRunner:
        """Build an installer from the checkout's `.gclient` config.

        This installer emulates DEPS as an environmnet. `_SRC_DIR.parent` is
        used as the starting point, so it resolves regardless of where the hook
        is invoked from. Starting at the parent is deliberate: `FindGclientRoot`
        walks upward and returns the first directory containing a `.gclient`,
        and the Chromium `src` checkout is never itself the gclient root.
        """
        parser = gclient.OptionParser()
        options, _ = parser.parse_args([])
        root = gclient_paths.FindGclientRoot(str(_SRC_DIR.parent),
                                             options.config_filename)
        if root is None:
            raise RuntimeError(
                f'Could not find a .gclient root from {_SRC_DIR.parent}')
        client = gclient.GClient(root, options)
        client.SetConfig(
            gclient_utils.FileRead(Path(root) / options.config_filename))
        return cls(client)

    def _solution_scope(self, name: str) -> dict[str, object]:
        """Parse a solution's DEPS once, caching its `vars` and `deps`.

        Returns a dict with two keys:

          * `vars` -- the exact dict gclient builds while processing that
            solution's DEPS during a sync: the DEPS-declared `vars` overlaid
            with `.gclient` custom_vars, plus the built-in vars (`host_os`,
            `host_cpu`, `checkout_*`, ...). Letting depot_tools load and merge
            everything means this script never has to know which variables exist
            or how to compute their values. Conditions resolve the same way they
            would with `sync`.
          * `deps` -- the raw `deps` mapping declared in the DEPS file, keyed by
            checkout-relative path. Used to cross-check that an overlay still
            targets the upstream archive currently pinned in DEPS.
        """
        if name not in self._scope_by_solution:
            solution = next(
                (d for d in self._client.dependencies if d.name == name), None)
            if solution is None:
                names = [d.name for d in self._client.dependencies]
                raise RuntimeError(
                    f'No gclient solution named {name!r} (have: {names})')
            # Resolve the solution's DEPS READ-ONLY. We deliberately do NOT call
            # `solution.ParseDepsFile()`, as that will have other side effects.
            # For example: for `gcs`-type deps, gclient's dependency processing
            # runs its first-class-GCS handling, which can `rmtree()` paths.
            builtin_vars = solution.get_builtin_vars()
            deps_file = _SRC_DIR.parent / solution.name / solution.deps_file
            local_scope = gclient_eval.Parse(
                deps_file.read_bytes().decode('utf-8'), str(deps_file),
                solution.custom_vars, builtin_vars)
            merged: dict[str, object] = dict(local_scope.get('vars', {}))
            merged.update(builtin_vars)
            merged.update(solution.custom_vars or {})
            self._scope_by_solution[name] = {
                'vars': merged,
                'deps': local_scope.get('deps', {}),
            }
        return self._scope_by_solution[name]

    def _solution_vars(self, name: str) -> dict[str, object]:
        """Return (and cache) the fully-resolved DEPS variables for a solution.
        """
        return self._solution_scope(name)['vars']

    def _validate_overlay_target(self, path: str, overlayed_on: str) -> None:
        """Fail unless DEPS still pins the upstream archive we overlay on.

        At the moment, the only entry in `EXTRA_DEPS` is the Rust/WASM
        toolchain, which is deployed as an overlay on top of the upstream
        Chromium rust toolchain. To ensure that we are not sliding away from the
        version deploying by upstream, and the version used in our overlay, we
        provide `overlayed_on` to validate that the archive we are downloading
        is still pinned against a corresponding upstream archive in DEPS.

        This is sanity check prevents us from silently applying an overlay on
        top of a different upstream version, which could easily happen whenever
        upstream rolls a new toolchain version.
        """
        deps_map = self._solution_scope(path.split('/', 1)[0])['deps']
        # gclient_eval parses deps into `_NodeDict` (a Mapping, NOT a `dict`
        # subclass), so check against Mapping rather than `dict`.
        dep = deps_map.get(path)
        if not isinstance(dep, Mapping) or dep.get('dep_type') != 'gcs':
            raise RuntimeError(
                f'Refusing to install {path}: it is not a `gcs` dependency in '
                f'DEPS, so the overlay base {overlayed_on!r} cannot be '
                f'verified.')
        objects = dep.get('objects') or []
        if not any(obj.get('object_name') == overlayed_on for obj in objects):
            available = sorted(str(obj.get('object_name')) for obj in objects)
            raise RuntimeError(
                f'Refusing to install {path}: DEPS does not pin the expected '
                f'overlay base {overlayed_on!r} (upstream may have rolled the '
                f'toolchain). Objects currently pinned in DEPS: {available}.')

    def install(self, path: str, spec: dict) -> None:
        """Download and install the matching object for one `EXTRA_DEPS` entry.
        """
        variables = self._solution_vars(path.split('/', 1)[0])

        condition = spec.get('condition')
        if condition and not gclient_eval.EvaluateCondition(
                condition, variables):
            _LOG.debug('Skipping %s: condition %r is false', path, condition)
            return

        obj = _select_object(spec['objects'], variables)
        if obj is None:
            _LOG.debug('No matching object for %s on this host', path)
            return

        # An overlay must sit on the base it was built against: validate it
        # still matches DEPS before touching the destination. No `overlayed_on`
        # means the dep owns its destination, so there is no base to validate.
        overlayed_on = obj.get('overlayed_on')
        if overlayed_on:
            self._validate_overlay_target(path, overlayed_on)

        installer = TarballInstaller(dest_dir=_SRC_DIR.parent / path,
                                     url=spec['bucket'] + obj['object_name'],
                                     object_name=obj['object_name'],
                                     sha256sum=obj['sha256sum'],
                                     owns_dest=not overlayed_on)
        if installer.install():
            _LOG.info('Installed %s into %s', obj['object_name'],
                      installer.dest_dir)
        else:
            _LOG.debug('%s already up to date (%s)', path, obj['object_name'])


def main() -> int:
    # The gclient machinery used to resolve DEPS conditions logs very verbosely
    # on the root logger (every dependency's `verify_validity`, recursedeps,
    # etc.). Keep the root at ERROR to silence that chatter, and emit this
    # script's own messages through `_LOG` (at INFO) so they still surface.
    logging.basicConfig(level=logging.ERROR, format='%(message)s')
    _LOG.setLevel(logging.INFO)

    parser = argparse.ArgumentParser(
        description='Download and install the bucket-hosted archive(s) for the '
        'given EXTRA_DEPS entries.')
    parser.add_argument(
        'deps',
        nargs='+',
        choices=sorted(EXTRA_DEPS),
        metavar='DEP_PATH',
        help='One or more path keys in EXTRA_DEPS identifying the entries to '
        'install. Entries whose condition is false on this host are skipped, '
        'so a single invocation may list every per-platform variant.')
    args = parser.parse_args()

    runner = ExtraDepsRunner.from_checkout()
    for dep in args.deps:
        runner.install(dep, EXTRA_DEPS[dep])
    return 0


if __name__ == '__main__':
    sys.exit(main())
