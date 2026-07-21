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
"""

from __future__ import annotations

import argparse
from collections.abc import Mapping
import logging
import sys
from pathlib import Path

# `src/` directory (this script lives at src/brave/tools/cr/).
_SRC_DIR = Path(__file__).resolve().parents[3]

# Importing depot_tools explicitly, so we can call this script from the terminal
# without needing to set up the PYTHONPATH. `depot_tools` is inserted at the
# front so its `third_party` package (which provides `third_party.schema`,
# needed by gclient_eval) is not shadowed by the `third_party` package in the
# vpython virtualenv.
sys.path.insert(0, str(_SRC_DIR / 'third_party' / 'depot_tools'))

import gclient  # pylint: disable=wrong-import-position,import-error
import gclient_eval  # pylint: disable=wrong-import-position,import-error
import gclient_paths  # pylint: disable=wrong-import-position,import-error
import gclient_utils  # pylint: disable=wrong-import-position,import-error

from extra_deps import EXTRA_DEPS  # pylint: disable=wrong-import-position
from tarball_installer import (  # pylint: disable=wrong-import-position
    TarballInstaller)

# This script's own logger, so we don't step on gclient's logger. It is also of
# notice that most logs are DEBUG. No-op runs should not produce any output in
# normal runs, to avoid cluttering the sync output.
_LOG = logging.getLogger('install_extra_deps')


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

          * `vars`: the exact dict gclient builds while processing that
            solution's DEPS during a sync: the DEPS-declared `vars` overlaid
            with `.gclient` custom_vars, plus the built-in vars (`host_os`,
            `host_cpu`, `checkout_*`, ...). Letting depot_tools load and merge
            everything means this script never has to know which variables exist
            or how to compute their values. Conditions resolve the same way they
            would with `sync`.
          * `deps`: the raw `deps` mapping declared in the DEPS file, keyed by
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

        overlayed_on = obj.get('overlayed_on')
        installer = TarballInstaller(dest_dir=_SRC_DIR.parent / path,
                                     url=spec['bucket'] + obj['object_name'],
                                     object_name=obj['object_name'],
                                     sha256sum=obj['sha256sum'],
                                     owns_dest=not overlayed_on)

        # An overlay must sit on the base it was built against: validate it
        # still matches DEPS before touching the destination. No `overlayed_on`
        # means the dep owns its destination, so there is no base to validate.
        if overlayed_on:
            self._validate_overlay_target(path, overlayed_on)

        # `install()` no-ops (returns False) when the sidecar already records
        # this object's sha, so there is no separate deployment check here.
        installer.install()


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
