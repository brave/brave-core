#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `TarballInstaller`.

Drives a single installer against real on-disk archives (built in-memory) with
the byte fetch injected, so no network is touched but the real sha verification,
extraction, owned-vs-overlay wipe, idempotency, and sidecar bookkeeping run.
"""

from __future__ import annotations

import contextlib
import hashlib
import io
import json
import sys
import tarfile
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

# `tarball_installer` (and the `extra_deps` it imports) live at the tools/cr
# root; add it so they resolve regardless of the working directory.
sys.path.insert(0, str(Path(__file__).resolve().parent))

import tarball_installer as m


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _make_tar(members: list[tuple[str, bytes]], compression: str = 'gz'):
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode=f'w:{compression}') as tar:
        for name, content in members:
            info = tarfile.TarInfo(name)
            info.size = len(content)
            tar.addfile(info, io.BytesIO(content))
    return buf.getvalue()


def _make_zip(members: list[tuple[str, bytes]]):
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, 'w') as archive:
        for name, content in members:
            archive.writestr(name, content)
    return buf.getvalue()


class TarballInstallerTest(unittest.TestCase):
    """Tests for `TarballInstaller` fetch/extract/sidecar mechanics."""

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

    def _download(self, data: bytes):
        """An injected fetch that writes `data` into the output file."""

        def _write(_url, output_file):
            output_file.write(data)

        return _write

    def test_install_extracts_tarball_and_writes_sidecars(self):
        data = _make_tar([('bin/node', b'node'), ('README.md', b'hi')])
        installer = self._installer(data)
        self.assertTrue(installer.install(self._download(data)))
        self.assertEqual((self.dest / 'bin/node').read_bytes(), b'node')
        self.assertEqual((self.dest / 'README.md').read_bytes(), b'hi')
        self.assertTrue(installer.is_installed())

    def test_install_extracts_zip(self):
        data = _make_zip([('node.exe', b'MZ'), ('LICENSE', b'mpl')])
        installer = self._installer(data, object_name='node.zip')
        self.assertTrue(installer.install(self._download(data)))
        self.assertEqual((self.dest / 'node.exe').read_bytes(), b'MZ')
        self.assertEqual((self.dest / 'LICENSE').read_bytes(), b'mpl')

    def test_install_is_idempotent(self):
        data = _make_tar([('f', b'x')])
        installer = self._installer(data)
        calls = []

        def _counting(url, output_file):
            calls.append(url)
            output_file.write(data)

        self.assertTrue(installer.install(_counting))
        self.assertFalse(installer.install(_counting))
        self.assertEqual(len(calls), 1)

    def test_is_installed_is_false_without_sidecar(self):
        self.assertFalse(
            self._installer(_make_tar([('f', b'x')])).is_installed())

    def test_owned_dest_is_wiped_before_extract(self):
        self.dest.mkdir()
        (self.dest / 'stale.txt').write_text('old')
        data = _make_tar([('fresh.txt', b'new')])
        self._installer(data, owns_dest=True).install(self._download(data))
        self.assertFalse((self.dest / 'stale.txt').exists())
        self.assertTrue((self.dest / 'fresh.txt').exists())

    def test_overlay_keeps_existing_tree(self):
        self.dest.mkdir()
        (self.dest / 'base.txt').write_text('upstream')
        data = _make_tar([('overlay.txt', b'brave')])
        self._installer(data, owns_dest=False).install(self._download(data))
        self.assertEqual((self.dest / 'base.txt').read_text(), 'upstream')
        self.assertTrue((self.dest / 'overlay.txt').exists())

    def test_sha256_mismatch_raises_and_installs_nothing(self):
        data = _make_tar([('f', b'x')])
        installer = self._installer(data, sha256sum='0' * 64)
        with self.assertRaises(ValueError):
            installer.install(self._download(data))
        self.assertFalse(self.dest.exists())
        self.assertFalse(installer.is_installed())

    def test_download_refuses_non_https_url(self):
        installer = self._installer(_make_tar([('f', b'x')]))
        with self.assertRaisesRegex(ValueError, 'non-https'):
            installer._download('http://downloads.invalid/pkg.tar.gz',
                                io.BytesIO())

    def test_sidecar_contents(self):
        members = [('a', b'1'), ('b/c', b'2')]
        data = _make_tar(members)
        installer = self._installer(data, object_name='pkg.tar.gz')
        installer.install(self._download(data))
        prefix = '.pkg_tar_gz'
        hash_file = self.dest / f'{prefix}_hash.stamp'
        names_file = self.dest / f'{prefix}_content_names.stamp'
        self.assertTrue(hash_file.is_file())
        self.assertEqual(hash_file.read_text().strip(), _sha256(data))
        self.assertEqual(json.loads(names_file.read_text()), ['a', 'b/c'])


class ProgressTest(unittest.TestCase):
    """Tests for the gsutil-style progress formatting."""

    def test_human_uses_base_1024_units(self):
        self.assertEqual(m.TarballInstaller._human(0), '0 B')
        self.assertEqual(m.TarballInstaller._human(28001234), '26.7 MiB')
        self.assertEqual(m.TarballInstaller._human(1024), '1 KiB')

    def test_download_writes_bytes_and_reports_done(self):
        """The real `_download` (no injected fetch) streams bytes and prints a
        final `Done` line -- guards the built-in path the fakes bypass."""

        class _FakeResponse:

            def __init__(self, data):
                self._buf = io.BytesIO(data)
                self.headers = {'Content-Length': str(len(data))}

            def read(self, size):
                return self._buf.read(size)

            def __enter__(self):
                return self

            def __exit__(self, *_):
                return False

        data = b'payload' * 5000
        out = io.BytesIO()
        inst = m.TarballInstaller(dest_dir=Path('/x'),
                                  url='https://downloads.invalid/pkg',
                                  object_name='pkg.tar.gz',
                                  sha256sum='a',
                                  owns_dest=True)
        err = io.StringIO()
        with mock.patch.object(m, 'urlopen', return_value=_FakeResponse(data)):
            with contextlib.redirect_stderr(err):
                inst._download('https://downloads.invalid/pkg', out)
        self.assertEqual(out.getvalue(), data)
        self.assertTrue(err.getvalue().endswith('Done\n'))

    def test_emit_progress_renders_live_line(self):
        inst = m.TarballInstaller(dest_dir=Path('/x'),
                                  url='u',
                                  object_name='node.tar.gz',
                                  sha256sum='a',
                                  owns_dest=True)
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            inst._emit_progress(26_700_000, 26_700_000, done_flag=True)
        # Carriage-return prefixed, `[done/total] pct% Done`, matching gsutil.
        self.assertEqual(err.getvalue(),
                         '\rnode.tar.gz [25.5 MiB/25.5 MiB] 100% Done\n')


class ForDepTest(unittest.TestCase):
    """Tests for the `TarballInstaller.for_dep` single-object factory."""

    def _spec(self, objects):
        return {'bucket': 'https://downloads.invalid/', 'objects': objects}

    def test_builds_single_object_installer(self):
        inst = m.TarballInstaller.for_dep(
            Path('/ws'), 'src/p',
            self._spec([{
                'object_name': 'x.tar.gz',
                'sha256sum': 'abc'
            }]))
        self.assertEqual(inst.dest_dir, Path('/ws/src/p'))
        self.assertEqual(inst.url, 'https://downloads.invalid/x.tar.gz')
        self.assertEqual(inst.object_name, 'x.tar.gz')
        self.assertEqual(inst.sha256sum, 'abc')
        self.assertTrue(inst.owns_dest)

    def test_overlay_object_does_not_own_dest(self):
        inst = m.TarballInstaller.for_dep(
            Path('/ws'), 'src/p',
            self._spec([{
                'object_name': 'x.tar.gz',
                'sha256sum': 'abc',
                'overlayed_on': 'base.tar.xz'
            }]))
        self.assertFalse(inst.owns_dest)

    def test_multi_object_entry_raises(self):
        spec = self._spec([
            {
                'object_name': 'a.tar.gz',
                'sha256sum': '1'
            },
            {
                'object_name': 'b.tar.gz',
                'sha256sum': '2'
            },
        ])
        with self.assertRaisesRegex(ValueError, 'single-object'):
            m.TarballInstaller.for_dep(Path('/ws'), 'src/p', spec)


class MainTest(unittest.TestCase):
    """Tests for the `tarball_installer.py` CLI."""

    def test_installs_the_requested_single_object_dep(self):
        dep = 'src/brave/third_party/node/node-linux-x64'
        with mock.patch.object(m.TarballInstaller,
                               'install',
                               autospec=True,
                               return_value=True) as install:
            self.assertEqual(m.main([dep]), 0)
        install.assert_called_once()
        installer = install.call_args.args[0]
        self.assertEqual(installer.dest_dir, m._WORKSPACE_ROOT / dep)
        self.assertEqual(installer.object_name,
                         'node-v24.18.0-linux-x64.tar.gz')

    def test_rejects_unknown_dep(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                m.main(['src/does/not/exist'])

    def test_rejects_multi_object_dep(self):
        # The rust toolchain has one object per host; argparse rejects it since
        # only single-object entries are installable here.
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                m.main(['src/third_party/rust-toolchain'])


if __name__ == '__main__':
    unittest.main()
