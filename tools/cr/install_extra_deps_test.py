#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `install_extra_deps`.

The file is split into layers, from pure units to a full checkout integration:

* `SelectObjectTest` covers the pure `_select_object` condition picker.

* `TarballInstallerTest` drives a single `TarballInstaller` against real
  on-disk archives (built in-memory). `deps.DownloadUrl` is faked so no network
  is touched, but the real `deps.VerifySHA256` and the real tar/zip extraction
  run, exercising download verification, the owned-vs-overlay wipe behaviour,
  idempotency, and the gclient-equivalent sidecar bookkeeping.

* `ValidateOverlayTargetTest` and `InstallTest` exercise `ExtraDepsRunner`
  without the gclient machinery by pre-seeding its resolved-scope cache, so the
  overlay-base validation and the install dispatch can be checked in isolation.

* `FromCheckoutIntegrationTest` drives `ExtraDepsRunner.from_checkout` against a
  `FakeChromiumRepo` with a minimal `.gclient` + `DEPS` written into it, so the
  real depot_tools `gclient`/`gclient_eval` stack resolves the conditions and
  the overlay base exactly as it would during a `gclient sync` -- end to end,
  still with the download faked.

* `MainTest` covers the `main()` CLI dispatch and argument validation.
"""

from __future__ import annotations

import contextlib
import hashlib
import io
import json
import logging
import sys
import tarfile
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

# `FakeChromiumRepo` lives in the `test` package at the `tools/cr` root; add it
# to the path so it resolves regardless of the working directory.
sys.path.insert(0, str(Path(__file__).resolve().parent))

import install_extra_deps as m
from test.fake_chromium_repo import FakeChromiumRepo


def _sha256(data: bytes) -> str:
    """The hex sha256 of `data`, as the installer records in its sidecar."""
    return hashlib.sha256(data).hexdigest()


def _make_tar(members: list[tuple[str, bytes]],
              compression: str = 'gz') -> bytes:
    """Build a tar archive (default gzip) from `(name, content)` members."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode=f'w:{compression}') as tar:
        for name, content in members:
            info = tarfile.TarInfo(name)
            info.size = len(content)
            tar.addfile(info, io.BytesIO(content))
    return buf.getvalue()


def _make_zip(members: list[tuple[str, bytes]]) -> bytes:
    """Build a zip archive from `(name, content)` members."""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, 'w') as archive:
        for name, content in members:
            archive.writestr(name, content)
    return buf.getvalue()


class SelectObjectTest(unittest.TestCase):
    """Tests for the `_select_object` per-host condition picker."""

    def test_returns_none_when_nothing_matches(self):
        """No object whose condition holds yields `None` (host not covered)."""
        objects = [{
            'object_name': 'linux.tar.gz',
            'condition': 'host_os == "linux"',
        }]
        self.assertIsNone(m._select_object(objects, {'host_os': 'mac'}))

    def test_returns_the_single_match(self):
        """Exactly one matching condition returns that object."""
        objects = [
            {
                'object_name': 'linux.tar.gz',
                'condition': 'host_os == "linux"'
            },
            {
                'object_name': 'mac.tar.gz',
                'condition': 'host_os == "mac"'
            },
        ]
        picked = m._select_object(objects, {'host_os': 'mac'})
        self.assertEqual(picked['object_name'], 'mac.tar.gz')

    def test_raises_when_multiple_match(self):
        """More than one matching object is ambiguous and must raise."""
        objects = [
            {
                'object_name': 'a.tar.gz',
                'condition': 'host_os == "linux"'
            },
            {
                'object_name': 'b.tar.gz',
                'condition': 'host_os == "linux"'
            },
        ]
        with self.assertRaisesRegex(RuntimeError, 'Multiple objects match'):
            m._select_object(objects, {'host_os': 'linux'})


class TarballInstallerTest(unittest.TestCase):
    """Tests for `TarballInstaller` download/extract/sidecar mechanics."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.dest = self.root / 'dest'

    def _installer(self,
                   data: bytes,
                   *,
                   object_name: str = 'pkg.tar.gz',
                   sha256sum: str | None = None,
                   owns_dest: bool = True) -> m.TarballInstaller:
        """A `TarballInstaller` for `data`, defaulting to its true sha256."""
        return m.TarballInstaller(
            dest_dir=self.dest,
            url=f'https://downloads.invalid/{object_name}',
            object_name=object_name,
            sha256sum=_sha256(data) if sha256sum is None else sha256sum,
            owns_dest=owns_dest)

    def _faked_download(self, data: bytes) -> mock._patch:
        """Patch `deps.DownloadUrl` to write `data` into the output file."""

        def _write(_url, output_file):
            output_file.write(data)

        return mock.patch.object(m.deps, 'DownloadUrl', side_effect=_write)

    def test_install_extracts_tarball_and_writes_sidecars(self):
        """A fresh install extracts the tar members and records the sidecars."""
        data = _make_tar([('bin/node', b'node'), ('README.md', b'hi')])
        installer = self._installer(data)
        with self._faked_download(data):
            self.assertTrue(installer.install())
        self.assertEqual((self.dest / 'bin/node').read_bytes(), b'node')
        self.assertEqual((self.dest / 'README.md').read_bytes(), b'hi')
        self.assertTrue(installer.is_installed())

    def test_install_extracts_zip(self):
        """Zip archives take the `ZipFile` branch and extract the same way."""
        data = _make_zip([('node.exe', b'MZ'), ('LICENSE', b'mpl')])
        installer = self._installer(data, object_name='node.zip')
        with self._faked_download(data):
            self.assertTrue(installer.install())
        self.assertEqual((self.dest / 'node.exe').read_bytes(), b'MZ')
        self.assertEqual((self.dest / 'LICENSE').read_bytes(), b'mpl')

    def test_install_is_idempotent(self):
        """A second install is a no-op: it returns False and re-downloads
        nothing once the `_hash` sidecar already records the sha."""
        data = _make_tar([('f', b'x')])
        installer = self._installer(data)
        with self._faked_download(data) as download:
            self.assertTrue(installer.install())
            self.assertFalse(installer.install())
            download.assert_called_once()

    def test_is_installed_is_false_without_sidecar(self):
        """With no `_hash` sidecar present nothing is considered installed."""
        self.assertFalse(
            self._installer(_make_tar([('f', b'x')])).is_installed())

    def test_owned_dest_is_wiped_before_extract(self):
        """An owned dep wipes its destination first, so stale files are gone."""
        self.dest.mkdir()
        (self.dest / 'stale.txt').write_text('old')
        data = _make_tar([('fresh.txt', b'new')])
        installer = self._installer(data, owns_dest=True)
        with self._faked_download(data):
            installer.install()
        self.assertFalse((self.dest / 'stale.txt').exists())
        self.assertTrue((self.dest / 'fresh.txt').exists())

    def test_overlay_keeps_existing_tree(self):
        """An overlay (owns_dest=False) extracts on top, preserving the base."""
        self.dest.mkdir()
        (self.dest / 'base.txt').write_text('upstream')
        data = _make_tar([('overlay.txt', b'brave')])
        installer = self._installer(data, owns_dest=False)
        with self._faked_download(data):
            installer.install()
        self.assertEqual((self.dest / 'base.txt').read_text(), 'upstream')
        self.assertTrue((self.dest / 'overlay.txt').exists())

    def test_sha256_mismatch_raises_and_installs_nothing(self):
        """A hash mismatch surfaces from the real `VerifySHA256` and leaves the
        destination untouched (no extraction, no sidecars)."""
        data = _make_tar([('f', b'x')])
        installer = self._installer(data, sha256sum='0' * 64)
        with self._faked_download(data):
            with self.assertRaises(ValueError):
                installer.install()
        self.assertFalse(self.dest.exists())
        self.assertFalse(installer.is_installed())

    def test_sidecar_contents(self):
        """The written sidecars mirror gclient's set: the sha256 and the JSON
        list of archive members, each `.stamp`."""
        members = [('a', b'1'), ('b/c', b'2')]
        data = _make_tar(members)
        installer = self._installer(data, object_name='pkg.tar.gz')
        with self._faked_download(data):
            installer.install()
        prefix = '.pkg_tar_gz'
        hash_file = self.dest / f'{prefix}_hash.stamp'
        names_file = self.dest / f'{prefix}_content_names.stamp'
        self.assertTrue(hash_file.is_file())
        self.assertEqual(hash_file.read_text().strip(), _sha256(data))
        self.assertEqual(json.loads(names_file.read_text()), ['a', 'b/c'])
        # The first-class-gcs migration toggle is deliberately not written.
        self.assertFalse(
            (self.dest / f'{prefix}_is_first_class_gcs.stamp').exists())

    def test_sidecar_name_maps_dots_and_slashes(self):
        """The sidecar prefix maps every `/` and `.` in the object name to `_`
        (matching gclient's keying) and carries the `.stamp` tail."""
        installer = self._installer(b'', object_name='Linux_x64/pkg.tar.gz')
        self.assertEqual(
            installer._sidecar('_hash').name,
            '.Linux_x64_pkg_tar_gz_hash.stamp')


class ValidateOverlayTargetTest(unittest.TestCase):
    """Tests for `ExtraDepsRunner._validate_overlay_target`.

    The runner's resolved-scope cache is seeded directly so these tests never
    touch gclient: only the DEPS cross-check logic is under test here.
    """

    PATH = 'src/third_party/rust-toolchain'
    BASE = 'Linux_x64/base.tar.xz'

    def _runner(self, deps_map: dict) -> m.ExtraDepsRunner:
        runner = m.ExtraDepsRunner(mock.Mock())
        runner._scope_by_solution['src'] = {'vars': {}, 'deps': deps_map}
        return runner

    def test_passes_when_deps_pins_the_base(self):
        """No error when DEPS still pins the expected overlay base as gcs."""
        runner = self._runner({
            self.PATH: {
                'dep_type': 'gcs',
                'objects': [{
                    'object_name': self.BASE
                }],
            }
        })
        runner._validate_overlay_target(self.PATH, self.BASE)  # no raise

    def test_raises_when_path_absent_from_deps(self):
        """A path missing from DEPS cannot be verified, so it must raise."""
        runner = self._runner({})
        with self.assertRaisesRegex(RuntimeError, 'not a `gcs` dependency'):
            runner._validate_overlay_target(self.PATH, self.BASE)

    def test_raises_when_dep_is_not_gcs(self):
        """A non-gcs dep at the path cannot anchor an overlay base."""
        runner = self._runner({self.PATH: {'dep_type': 'git'}})
        with self.assertRaisesRegex(RuntimeError, 'not a `gcs` dependency'):
            runner._validate_overlay_target(self.PATH, self.BASE)

    def test_raises_when_base_not_pinned(self):
        """A gcs dep that no longer pins the expected base must raise (upstream
        likely rolled the toolchain)."""
        runner = self._runner({
            self.PATH: {
                'dep_type': 'gcs',
                'objects': [{
                    'object_name': 'Linux_x64/rolled.tar.xz'
                }],
            }
        })
        with self.assertRaisesRegex(RuntimeError, 'does not pin the expected'):
            runner._validate_overlay_target(self.PATH, self.BASE)


class InstallTest(unittest.TestCase):
    """Tests for `ExtraDepsRunner.install` dispatch.

    The resolved-scope cache is seeded directly (no gclient), `_SRC_DIR` is
    pointed at a temp tree so destinations land there, and the download is
    faked, so the focus is purely the skip/select/validate/install branching.
    """

    def setUp(self):
        # Silence the script's own INFO line ("Installed ... into ...").
        logging.disable(logging.WARNING)
        self.addCleanup(logging.disable, logging.NOTSET)
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        # `install` derives dest as `_SRC_DIR.parent / path`; with _SRC_DIR at
        # <root>/src, a 'src/...' path resolves under <root>/src/...
        patcher = mock.patch.object(m, '_SRC_DIR', self.root / 'src')
        patcher.start()
        self.addCleanup(patcher.stop)

    def _runner(self,
                variables: dict,
                deps_map: dict | None = None) -> m.ExtraDepsRunner:
        runner = m.ExtraDepsRunner(mock.Mock())
        runner._scope_by_solution['src'] = {
            'vars': variables,
            'deps': deps_map or {},
        }
        return runner

    def _faked_download(self, data: bytes) -> mock._patch:

        def _write(_url, output_file):
            output_file.write(data)

        return mock.patch.object(m.deps, 'DownloadUrl', side_effect=_write)

    def test_skips_when_dep_condition_is_false(self):
        """A false dep-level condition skips entirely; nothing downloads."""
        runner = self._runner({'checkout_linux': False})
        spec = {
            'bucket': 'https://x/',
            'condition': 'checkout_linux',
            'objects': [{
                'object_name': 'pkg.tar.gz',
                'sha256sum': 'a',
                'condition': 'True'
            }],
        }
        with self._faked_download(b'') as download:
            runner.install('src/foo', spec)
            download.assert_not_called()

    def test_skips_when_no_object_matches_host(self):
        """When no per-object condition matches this host, nothing downloads."""
        runner = self._runner({'host_os': 'linux'})
        spec = {
            'bucket': 'https://x/',
            'objects': [{
                'object_name': 'mac.tar.gz',
                'sha256sum': 'a',
                'condition': 'host_os == "mac"'
            }],
        }
        with self._faked_download(b'') as download:
            runner.install('src/foo', spec)
            download.assert_not_called()

    def test_installs_owned_object_without_overlay_validation(self):
        """An owned object installs straight through; the overlay validation is
        never consulted (there is no base to check)."""
        data = _make_tar([('bin/node', b'node')])
        runner = self._runner({'host_os': 'linux'})
        spec = {
            'bucket': 'https://downloads.invalid/',
            'objects': [{
                'object_name': 'node.tar.gz',
                'sha256sum': _sha256(data),
                'condition': 'host_os == "linux"'
            }],
        }
        with mock.patch.object(runner, '_validate_overlay_target') as validate:
            with self._faked_download(data):
                runner.install('src/brave/third_party/node', spec)
            validate.assert_not_called()
        dest = self.root / 'src/brave/third_party/node'
        self.assertEqual((dest / 'bin/node').read_bytes(), b'node')

    def test_validates_overlay_base_before_installing(self):
        """An overlay object validates its base against DEPS before extracting,
        passing the object's `overlayed_on` to the check."""
        data = _make_tar([('lib/std', b'rlib')])
        runner = self._runner({'host_os': 'linux'})
        spec = {
            'bucket': 'https://downloads.invalid/',
            'objects': [{
                'object_name': 'overlay.tar.gz',
                'sha256sum': _sha256(data),
                'overlayed_on': 'Linux_x64/base.tar.xz',
                'condition': 'host_os == "linux"',
            }],
        }
        with mock.patch.object(runner, '_validate_overlay_target') as validate:
            with self._faked_download(data):
                runner.install('src/third_party/rust-toolchain', spec)
            validate.assert_called_once_with('src/third_party/rust-toolchain',
                                             'Linux_x64/base.tar.xz')

    def test_overlay_validation_failure_prevents_download(self):
        """If the overlay base no longer validates, the install aborts before
        touching the network."""
        runner = self._runner({'host_os': 'linux'})
        spec = {
            'bucket': 'https://downloads.invalid/',
            'objects': [{
                'object_name': 'overlay.tar.gz',
                'sha256sum': 'a',
                'overlayed_on': 'Linux_x64/base.tar.xz',
                'condition': 'host_os == "linux"',
            }],
        }
        with mock.patch.object(runner,
                               '_validate_overlay_target',
                               side_effect=RuntimeError('rolled')):
            with self._faked_download(b'') as download:
                with self.assertRaises(RuntimeError):
                    runner.install('src/third_party/rust-toolchain', spec)
                download.assert_not_called()


class FromCheckoutIntegrationTest(unittest.TestCase):
    """End-to-end tests over a fake checkout with the real gclient stack.

    A `FakeChromiumRepo` provides the `src/` layout; a minimal `.gclient` and
    `DEPS` written into it let depot_tools resolve variables and the overlay
    base exactly as a real `gclient sync` would. `host_os`/`host_cpu` are pinned
    via `.gclient` custom_vars so conditions resolve deterministically on any
    test host. Only the download is faked.
    """

    def setUp(self):
        # gclient logs very verbosely on the root logger while resolving DEPS;
        # silence it (and the script's own INFO line) for clean test output.
        logging.disable(logging.WARNING)
        self.addCleanup(logging.disable, logging.NOTSET)

        self.repo = FakeChromiumRepo()
        self.addCleanup(self.repo.cleanup)
        (self.repo.base_path / '.gclient').write_text(
            'solutions = [{'
            '"name": "src", '
            '"url": "https://example.invalid/src.git", '
            '"custom_vars": {'
            '"host_os": "linux", "host_cpu": "x64", "checkout_linux": True}}]\n'
        )
        (self.repo.chromium / 'DEPS').write_text(
            "deps = {\n"
            "  'src/third_party/rust-toolchain': {\n"
            "    'dep_type': 'gcs', 'bucket': 'chromium-bucket',\n"
            "    'objects': [{'object_name': 'Linux_x64/base.tar.xz',\n"
            "                 'sha256sum': 'aa', 'size_bytes': 1,\n"
            "                 'generation': 1}],\n"
            "  },\n"
            "}\n")
        patcher = mock.patch.object(m, '_SRC_DIR', self.repo.chromium)
        patcher.start()
        self.addCleanup(patcher.stop)

    def _faked_download(self, data: bytes) -> mock._patch:

        def _write(_url, output_file):
            output_file.write(data)

        return mock.patch.object(m.deps, 'DownloadUrl', side_effect=_write)

    def test_resolves_solution_vars_and_caches(self):
        """`from_checkout` loads the config and resolves the solution's vars,
        merging in the `.gclient` custom_vars and caching the parsed scope."""
        runner = m.ExtraDepsRunner.from_checkout()
        variables = runner._solution_vars('src')
        self.assertEqual(variables['host_os'], 'linux')
        self.assertEqual(variables['host_cpu'], 'x64')
        self.assertTrue(variables['checkout_linux'])
        # Second lookup hits the cache and returns the very same scope object.
        self.assertIs(runner._solution_scope('src'),
                      runner._solution_scope('src'))

    def test_unknown_solution_raises(self):
        """Asking for a solution the `.gclient` does not declare is an error."""
        runner = m.ExtraDepsRunner.from_checkout()
        with self.assertRaisesRegex(RuntimeError, 'No gclient solution'):
            runner._solution_vars('nonexistent')

    def test_missing_gclient_root_raises(self):
        """`from_checkout` raises when no `.gclient` is found walking upward."""
        with tempfile.TemporaryDirectory() as orphan:
            with mock.patch.object(m, '_SRC_DIR', Path(orphan) / 'src'):
                with self.assertRaisesRegex(RuntimeError,
                                            'Could not find a .gclient root'):
                    m.ExtraDepsRunner.from_checkout()

    def test_validate_overlay_target_against_real_deps(self):
        """The overlay base resolves against the DEPS parsed from the checkout:
        the pinned base passes, a different one raises."""
        runner = m.ExtraDepsRunner.from_checkout()
        runner._validate_overlay_target('src/third_party/rust-toolchain',
                                        'Linux_x64/base.tar.xz')  # no raise
        with self.assertRaisesRegex(RuntimeError, 'does not pin the expected'):
            runner._validate_overlay_target('src/third_party/rust-toolchain',
                                            'Linux_x64/rolled.tar.xz')

    def test_install_end_to_end(self):
        """A full install resolves the host condition through gclient, extracts
        the archive into the checkout path, and writes the sidecars."""
        data = _make_tar([('bin/node', b'node')])
        spec = {
            'bucket': 'https://downloads.invalid/',
            'condition': 'checkout_linux',
            'objects': [
                {
                    'object_name': 'node-linux.tar.gz',
                    'sha256sum': _sha256(data),
                    'condition': 'host_os == "linux"',
                },
                {
                    'object_name': 'node-mac.tar.gz',
                    'sha256sum': 'unused',
                    'condition': 'host_os == "mac"',
                },
            ],
        }
        runner = m.ExtraDepsRunner.from_checkout()
        with self._faked_download(data):
            runner.install('src/brave/third_party/node', spec)
        dest = self.repo.chromium / 'brave/third_party/node'
        self.assertEqual((dest / 'bin/node').read_bytes(), b'node')
        self.assertTrue((dest / '.node-linux_tar_gz_hash.stamp').is_file())

    def test_install_skips_when_dep_condition_is_false(self):
        """A dep-level condition that is false for the checkout installs
        nothing, even though an object would otherwise match the host."""
        data = _make_tar([('f', b'x')])
        spec = {
            'bucket': 'https://downloads.invalid/',
            'condition': 'checkout_android',
            'objects': [{
                'object_name': 'node.tar.gz',
                'sha256sum': _sha256(data),
                'condition': 'host_os == "linux"',
            }],
        }
        runner = m.ExtraDepsRunner.from_checkout()
        with self._faked_download(data) as download:
            runner.install('src/brave/third_party/node', spec)
            download.assert_not_called()
        self.assertFalse(
            (self.repo.chromium / 'brave/third_party/node').exists())


class MainTest(unittest.TestCase):
    """Tests for the `main()` CLI dispatch.

    `main()` reads `EXTRA_DEPS` for both the argparse `choices` and the spec
    lookup, so these tests patch in their own fixture rather than depending on
    whatever the live table happens to declare.
    """

    _FAKE_EXTRA_DEPS = {
        'src/path/to/dep': {
            'bucket': 'https://downloads.invalid/',
            'objects': [{
                'object_name': 'pkg.tar.gz',
                'sha256sum': 'a',
                'condition': 'True',
            }],
        },
    }

    def test_dispatches_selected_dep_to_runner(self):
        """A valid dep key resolves a runner and installs that entry's spec."""
        runner = mock.Mock()
        dep = 'src/path/to/dep'
        with mock.patch.object(m, 'EXTRA_DEPS', self._FAKE_EXTRA_DEPS):
            with mock.patch.object(m.ExtraDepsRunner,
                                   'from_checkout',
                                   return_value=runner):
                with mock.patch.object(sys, 'argv',
                                       ['install_extra_deps', dep]):
                    self.assertEqual(m.main(), 0)
        runner.install.assert_called_once_with(dep, self._FAKE_EXTRA_DEPS[dep])

    def test_rejects_unknown_dep(self):
        """An unknown dep key is rejected by argparse before any work runs."""
        with mock.patch.object(m, 'EXTRA_DEPS', self._FAKE_EXTRA_DEPS):
            with mock.patch.object(m.ExtraDepsRunner,
                                   'from_checkout') as checkout:
                with mock.patch.object(
                        sys, 'argv',
                    ['install_extra_deps', 'src/does/not/exist']):
                    # argparse prints a usage error to stderr before exiting;
                    # keep it out of the test output.
                    with contextlib.redirect_stderr(io.StringIO()):
                        with self.assertRaises(SystemExit):
                            m.main()
                checkout.assert_not_called()


if __name__ == '__main__':
    unittest.main()
