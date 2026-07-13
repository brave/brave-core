#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the stdlib-only `extra_deps` module.

Covers the sidecar helpers and the gclient-free `check_extra_deps_installed` check,
plus a guard that the node entries stay cheaply checkable (single-object), as
the bootstrap shims rely on.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

# `extra_deps` lives beside this test at the `tools/cr` root.
sys.path.insert(0, str(Path(__file__).resolve().parent))

import extra_deps as m


class SidecarTest(unittest.TestCase):
    """Tests for `sidecar_path` and the `is_deployed` sidecar check."""

    def test_sidecar_path_maps_dots_and_slashes(self):
        """The sidecar prefix maps every `/` and `.` to `_` (matching gclient's
        keying) and carries the `.stamp` tail."""
        self.assertEqual(
            m.sidecar_path(Path('/d'), 'Linux_x64/pkg.tar.gz', '_hash').name,
            '.Linux_x64_pkg_tar_gz_hash.stamp')

    def test_is_deployed_true_when_hash_matches(self):
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp)
            m.sidecar_path(dest, 'pkg.tar.gz', '_hash').write_text('abc\n')
            self.assertTrue(m.is_deployed(dest, 'pkg.tar.gz', 'abc'))

    def test_is_deployed_false_without_sidecar(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertFalse(m.is_deployed(Path(tmp), 'pkg.tar.gz', 'abc'))

    def test_is_deployed_false_on_hash_mismatch(self):
        """A sidecar recording a different sha (a rolled version) is stale."""
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp)
            m.sidecar_path(dest, 'pkg.tar.gz', '_hash').write_text('old\n')
            self.assertFalse(m.is_deployed(dest, 'pkg.tar.gz', 'new'))


class CheapInstallStateTest(unittest.TestCase):
    """Tests for the gclient-free `check_extra_deps_installed` sidecar check.

    A temp dir stands in for the workspace root, so the sidecar lookup lands
    there. No conditions are resolved: the check answers purely from the sidecar
    under `root / path`.
    """

    PATH = 'src/brave/third_party/node/node-linux-x64'

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        # The workspace root (the parent of `src`).
        self.root = Path(tmp.name)

    def _spec(self,
              *,
              objects: list[dict] | None = None,
              object_name: str = 'node.tar.gz',
              sha256sum: str = 'abc') -> dict:
        if objects is None:
            objects = [{'object_name': object_name, 'sha256sum': sha256sum}]
        return {'bucket': 'https://downloads.invalid/', 'objects': objects}

    def _seed(self, spec: dict) -> None:
        """Write the `_hash` sidecar so the entry reads as already deployed."""
        obj = spec['objects'][0]
        dest = self.root / self.PATH
        dest.mkdir(parents=True, exist_ok=True)
        m.sidecar_path(dest, obj['object_name'],
                       '_hash').write_text(obj['sha256sum'] + '\n')

    def _check(self, spec: dict) -> bool:
        """`check_extra_deps_installed` looks the entry up in `EXTRA_DEPS`, so
        patch the table to `spec` for `PATH`."""
        with mock.patch.object(m, 'EXTRA_DEPS', {self.PATH: spec}):
            return m.check_extra_deps_installed(self.root, self.PATH)

    def test_installed_returns_true(self):
        spec = self._spec()
        self._seed(spec)
        self.assertTrue(self._check(spec))

    def test_missing_returns_false(self):
        self.assertFalse(self._check(self._spec()))

    def test_hash_mismatch_returns_false(self):
        self._seed(self._spec(sha256sum='old'))
        self.assertFalse(self._check(self._spec(sha256sum='new')))

    def test_unknown_entry_raises(self):
        with mock.patch.object(m, 'EXTRA_DEPS', {}):
            with self.assertRaises(KeyError):
                m.check_extra_deps_installed(self.root, self.PATH)

    def test_multiple_objects_raises(self):
        spec = self._spec(objects=[
            {
                'object_name': 'a.tar.gz',
                'sha256sum': 'a',
                'condition': 'host_os == "linux"'
            },
            {
                'object_name': 'b.tar.gz',
                'sha256sum': 'b',
                'condition': 'host_os == "mac"'
            },
        ])
        with self.assertRaisesRegex(ValueError, 'single-object'):
            self._check(spec)


class ExtraDepsTableTest(unittest.TestCase):
    """Guards on the shared `EXTRA_DEPS` table itself."""

    def test_node_entries_are_single_object(self):
        """The bootstrap shims check node via `check_extra_deps_installed`, which only
        supports single-object entries, so the node entries must stay so."""
        node_entries = [path for path in m.EXTRA_DEPS if '/node/' in path]
        self.assertTrue(node_entries)
        for path in node_entries:
            self.assertEqual(len(m.EXTRA_DEPS[path]['objects']), 1, path)

    def test_all_buckets_are_https(self):
        """Downloads must be over https; the installer refuses anything else."""
        for path, spec in m.EXTRA_DEPS.items():
            self.assertTrue(spec['bucket'].startswith('https://'), path)


if __name__ == '__main__':
    unittest.main()
