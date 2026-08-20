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
import os
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


def _make_tar(members: list, compression: str = 'gz'):
    """A tarball of `(name, content)` pairs and ready-made `TarInfo` members."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode=f'w:{compression}') as tar:
        for member in members:
            if isinstance(member, tarfile.TarInfo):
                tar.addfile(member)
                continue
            name, content = member
            info = tarfile.TarInfo(name)
            info.size = len(content)
            tar.addfile(info, io.BytesIO(content))
    return buf.getvalue()


def _tar_member(name: str,
                member_type: bytes,
                linkname: str = '') -> tarfile.TarInfo:
    """A non-regular member (symlink, hardlink, device) for `_make_tar`."""
    info = tarfile.TarInfo(name)
    info.type = member_type
    info.linkname = linkname
    return info


def _can_symlink() -> bool:
    """Whether this runtime can create a symlink at all.

    Windows needs either administrator rights or Developer Mode for that, so it
    is a property of the machine rather than of the platform -- probe for it
    (the way CPython's own `test.support.can_symlink` does) so a box that can
    make symlinks exercises them instead of skipping. `tarfile` quietly falls
    back to copying a link's target when it cannot create one, so the tests
    that assert on a symlink need the real thing to be meaningful.
    """
    with tempfile.TemporaryDirectory() as tmp:
        try:
            Path(tmp, 'link').symlink_to(Path(tmp, 'target'))
        except (OSError, NotImplementedError):
            return False
        return True


_CAN_SYMLINK = _can_symlink()


def _make_zip(members: list[tuple[str, bytes]]):
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, 'w') as archive:
        for name, content in members:
            archive.writestr(name, content)
    return buf.getvalue()


def _make_zip_with_unix_modes(members: list[tuple[str, bytes, int]]):
    """A zip whose entries carry the given Unix `st_mode` (e.g. `0o100755`
    for an executable file, `0o120777` for a symlink), packed into the high
    16 bits of `external_attr` the way `zip`/`unzip` do."""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, 'w') as archive:
        for name, content, mode in members:
            info = zipfile.ZipInfo(name)
            info.external_attr = mode << 16
            archive.writestr(info, content)
    return buf.getvalue()


@contextlib.contextmanager
def _simulate_pre_pep706_python():
    """Make the runtime look like a `tarfile` without the PEP 706 filters.

    Drops the `data_filter` sentinel the installer probes for and makes
    `extractall` reject the `filter` kwarg, mimicking Python 3.12 predecessors
    (and pre-backport 3.8-3.11) so a regression to an unconditional `filter=`
    call would fail here.
    """
    saved = {
        name: getattr(tarfile, name)
        for name in ('data_filter', ) if hasattr(tarfile, name)
    }
    real_extractall = tarfile.TarFile.extractall

    def _reject_filter(tar_self, *args, **kwargs):
        if 'filter' in kwargs:
            raise TypeError(
                "extractall() got an unexpected keyword argument 'filter'")
        return real_extractall(tar_self, *args, **kwargs)

    for name in saved:
        delattr(tarfile, name)
    with mock.patch.object(tarfile.TarFile, 'extractall', _reject_filter):
        try:
            yield
        finally:
            for name, value in saved.items():
                setattr(tarfile, name, value)


@contextlib.contextmanager
def _simulate_pre_gh107845_data_filter():
    """Make `filter='data'` behave the way Python 3.10.12's does.

    That first PEP 706 backport resolves a symlink's target against the
    destination root instead of against the directory holding the link, so an
    ordinary `bin/corepack -> ../lib/node_modules/...` is rejected as escaping
    the destination. Upstream fixed it in 3.10.13 (gh-107845), but the bare
    `python3` our Linux builders run is older than that, so extraction must not
    depend on the filter being correct.
    """

    # Both of these exist only on a runtime that has PEP 706 -- exactly what
    # the tests using this helper skip on -- so they are reached by name: our
    # linter runs against a `tarfile` that predates the filters and would
    # otherwise report them as missing members.
    link_error = getattr(tarfile, 'LinkOutsideDestinationError')
    named_filters = getattr(tarfile, '_NAMED_FILTERS')

    def _buggy_data_filter(member, dest_path):
        if (member.issym()
                or member.islnk()) and not os.path.isabs(member.linkname):
            dest_path = os.path.realpath(dest_path)
            target = os.path.realpath(os.path.join(dest_path, member.linkname))
            if os.path.commonpath([target, dest_path]) != dest_path:
                raise link_error(member, target)
        return member

    with mock.patch.dict(named_filters, {'data': _buggy_data_filter}):
        yield


class TarballInstallerTest(unittest.TestCase):
    """Tests for `TarballInstaller` fetch/extract/sidecar mechanics."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name).resolve()
        self.dest = self.root / 'dest'

    def _installer(self,
                   data: bytes,
                   *,
                   object_name: str = 'pkg.tar.gz',
                   sha256sum: str | None = None,
                   size_bytes: int | None = None,
                   owns_dest: bool = True) -> m.TarballInstaller:
        """A `TarballInstaller` for `data`, defaulting to its true size/sha256."""
        return m.TarballInstaller(
            dest_dir=self.dest,
            url=f'https://example.com/{object_name}',
            object_name=object_name,
            sha256sum=_sha256(data) if sha256sum is None else sha256sum,
            size_bytes=len(data) if size_bytes is None else size_bytes,
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

    def test_install_extracts_tarball_without_pep706_filter(self):
        """Extraction still works on a Python that lacks the `filter='data'`
        guard (this script runs under whatever bare `python3` is on $PATH)."""
        data = _make_tar([('bin/node', b'node'), ('README.md', b'hi')])
        installer = self._installer(data)
        with _simulate_pre_pep706_python():
            self.assertTrue(installer.install(self._download(data)))
        self.assertEqual((self.dest / 'bin/node').read_bytes(), b'node')
        self.assertEqual((self.dest / 'README.md').read_bytes(), b'hi')
        self.assertTrue(installer.is_installed())

    # The shape every Node tarball has: `bin/corepack` is a symlink into the
    # sibling `lib/` tree, so its target climbs out of `bin/` while staying
    # inside the destination.
    _NODE_LIKE_MEMBERS = [
        ('lib/node_modules/corepack/dist/corepack.js', b'corepack'),
        _tar_member('bin/corepack', tarfile.SYMTYPE,
                    '../lib/node_modules/corepack/dist/corepack.js'),
    ]

    @unittest.skipUnless(_CAN_SYMLINK, 'this machine cannot create symlinks')
    def test_install_keeps_symlinks_climbing_out_of_their_directory(self):
        data = _make_tar(self._NODE_LIKE_MEMBERS)
        installer = self._installer(data)
        self.assertTrue(installer.install(self._download(data)))
        link = self.dest / 'bin' / 'corepack'
        self.assertTrue(link.is_symlink())
        self.assertEqual(link.read_bytes(), b'corepack')

    @unittest.skipUnless(_CAN_SYMLINK, 'this machine cannot create symlinks')
    @unittest.skipIf(not hasattr(tarfile, 'data_filter'),
                     'runtime has no PEP 706 filters to mis-behave')
    def test_install_does_not_depend_on_the_data_filter(self):
        """Extraction works even where `filter='data'` wrongly rejects a
        relative symlink -- the failure our Linux builders hit, since they run
        `launcher.py` (and so this installer) under a bare Python 3.10.12."""
        data = _make_tar(self._NODE_LIKE_MEMBERS)
        installer = self._installer(data)
        with _simulate_pre_gh107845_data_filter():
            self.assertTrue(installer.install(self._download(data)))
        self.assertTrue((self.dest / 'bin' / 'corepack').is_symlink())

    def test_install_rejects_member_outside_the_destination(self):
        data = _make_tar([('../escape', b'x')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'outside'):
            installer.install(self._download(data))
        self.assertFalse(installer.is_installed())

    def test_install_rejects_absolute_member(self):
        data = _make_tar([('/etc/passwd', b'x')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'absolute path'):
            installer.install(self._download(data))

    # The two shapes that only escape on Windows. Both are checked lexically,
    # so they are refused on every platform rather than only where they bite --
    # a Windows-only escape must not sail through review on a posix bot.
    def test_install_rejects_member_with_a_windows_separator(self):
        # A backslash is an ordinary character in a posix member name, so
        # `..\evil` looks contained here and climbs out on Windows.
        data = _make_tar([('..\\evil', b'x')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'backslash'):
            installer.install(self._download(data))

    def test_install_rejects_member_with_a_drive_letter(self):
        # `C:/evil` is absolute on Windows while looking relative on posix.
        data = _make_tar([('C:/evil', b'x')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'absolute path'):
            installer.install(self._download(data))

    def test_install_rejects_symlink_target_with_a_windows_separator(self):
        data = _make_tar(
            [_tar_member('bin/evil', tarfile.SYMTYPE, '..\\..\\evil')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'backslash'):
            installer.install(self._download(data))

    def test_install_rejects_symlink_pointing_outside(self):
        data = _make_tar(
            [_tar_member('bin/evil', tarfile.SYMTYPE, '../../../etc/passwd')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'outside'):
            installer.install(self._download(data))

    def test_install_rejects_absolute_symlink_target(self):
        data = _make_tar(
            [_tar_member('bin/evil', tarfile.SYMTYPE, '/etc/passwd')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'absolute path'):
            installer.install(self._download(data))

    def test_install_rejects_hardlink_pointing_outside(self):
        data = _make_tar(
            [_tar_member('bin/evil', tarfile.LNKTYPE, '../../etc/passwd')])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'outside'):
            installer.install(self._download(data))

    def test_install_rejects_device_member(self):
        # Extraction is fully-trusted, so a device or FIFO member would
        # otherwise be created verbatim.
        data = _make_tar([_tar_member('dev/null', tarfile.CHRTYPE)])
        installer = self._installer(data)
        with self.assertRaisesRegex(ValueError, 'device/FIFO'):
            installer.install(self._download(data))

    def test_install_allows_a_member_starting_with_two_dots(self):
        # `..stale` is an ordinary name, not a climb out of the destination.
        data = _make_tar([('..stale', b'x')])
        installer = self._installer(data)
        self.assertTrue(installer.install(self._download(data)))
        self.assertEqual((self.dest / '..stale').read_bytes(), b'x')

    def test_install_extracts_zip(self):
        data = _make_zip([('node.exe', b'MZ'), ('LICENSE', b'mpl')])
        installer = self._installer(data, object_name='node.zip')
        self.assertTrue(installer.install(self._download(data)))
        self.assertEqual((self.dest / 'node.exe').read_bytes(), b'MZ')
        self.assertEqual((self.dest / 'LICENSE').read_bytes(), b'mpl')

    @unittest.skipIf(sys.platform == 'win32', 'unzip is not used on Windows')
    def test_install_extracts_zip_preserves_mode_bits_and_symlinks(self):
        # `zipfile.extractall` drops the Unix permission bits zip stores in
        # `external_attr` and writes symlinks out as regular files holding the
        # link target as text -- exactly what BraveUpdater-*.zip needs kept
        # (mode-0755 executables plus a `ksadmin` symlink) for the updater
        # bundle to still run after extraction.
        data = _make_zip_with_unix_modes([
            ('bin/tool', b'#!/bin/sh\n', 0o100755),
            ('bin/link', b'tool', 0o120777),
        ])
        installer = self._installer(data, object_name='pkg.zip')
        self.assertTrue(installer.install(self._download(data)))
        tool = self.dest / 'bin/tool'
        link = self.dest / 'bin/link'
        self.assertTrue(os.access(tool, os.X_OK))
        self.assertTrue(link.is_symlink())
        self.assertEqual(os.readlink(link), 'tool')

    def test_install_extracts_zip_creates_missing_nested_dest(self):
        # `unzip -d` (unlike `zipfile.extractall`) only creates a single
        # missing directory level, so a `dest_dir` nested under parents that
        # don't exist yet must be created before `unzip` runs.
        self.dest = self.root / 'nested' / 'missing' / 'dest'
        data = _make_zip([('f', b'x')])
        installer = self._installer(data, object_name='pkg.zip')
        self.assertTrue(installer.install(self._download(data)))
        self.assertEqual((self.dest / 'f').read_bytes(), b'x')

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

    def test_size_mismatch_raises_and_installs_nothing(self):
        data = _make_tar([('f', b'x')])
        # Right bytes (so the sha would pass), but the pinned size is wrong: the
        # size check must fire first and abort before anything is extracted.
        installer = self._installer(data, size_bytes=len(data) + 1)
        with self.assertRaisesRegex(ValueError, 'size mismatch'):
            installer.install(self._download(data))
        self.assertFalse(self.dest.exists())
        self.assertFalse(installer.is_installed())

    def test_symlinked_owned_dest_is_replaced(self):
        # A symlinked destination is unlinked and replaced with the real
        # extracted tree (the hermetic Xcode script relies on this to overwrite
        # a symlink to a system SDK).
        data = _make_tar([('f', b'x')])
        elsewhere = self.root / 'elsewhere'
        elsewhere.mkdir()
        self.dest.symlink_to(elsewhere)
        self._installer(data, owns_dest=True).install(self._download(data))
        self.assertFalse(self.dest.is_symlink())
        self.assertTrue((self.dest / 'f').is_file())

    def test_download_refuses_non_https_url(self):
        installer = self._installer(_make_tar([('f', b'x')]))
        with self.assertRaisesRegex(ValueError, 'non-https'):
            installer._download('http://example.com/pkg.tar.gz', io.BytesIO())

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
                                  url='https://example.com/pkg',
                                  object_name='pkg.tar.gz',
                                  sha256sum='a',
                                  size_bytes=len(data),
                                  owns_dest=True)
        err = io.StringIO()
        with mock.patch.object(m, 'urlopen', return_value=_FakeResponse(data)):
            with contextlib.redirect_stderr(err):
                inst._download('https://example.com/pkg', out)
        self.assertEqual(out.getvalue(), data)
        self.assertTrue(err.getvalue().endswith('Done\n'))

    def test_emit_progress_renders_live_line(self):
        inst = m.TarballInstaller(dest_dir=Path('/x'),
                                  url='u',
                                  object_name='node.tar.gz',
                                  sha256sum='a',
                                  size_bytes=0,
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
        return {'bucket': 'https://example.com/', 'objects': objects}

    def test_builds_single_object_installer(self):
        inst = m.TarballInstaller.for_dep(
            Path('/ws'), 'src/p',
            self._spec([{
                'object_name': 'x.tar.gz',
                'sha256sum': 'abc',
                'size_bytes': 123
            }]))
        self.assertEqual(inst.dest_dir, Path('/ws/src/p'))
        self.assertEqual(inst.url, 'https://example.com/x.tar.gz')
        self.assertEqual(inst.object_name, 'x.tar.gz')
        self.assertEqual(inst.sha256sum, 'abc')
        self.assertEqual(inst.size_bytes, 123)
        self.assertTrue(inst.owns_dest)

    def test_overlay_object_does_not_own_dest(self):
        inst = m.TarballInstaller.for_dep(
            Path('/ws'), 'src/p',
            self._spec([{
                'object_name': 'x.tar.gz',
                'sha256sum': 'abc',
                'size_bytes': 123,
                'overlayed_on': 'base.tar.xz'
            }]))
        self.assertFalse(inst.owns_dest)

    def test_multi_object_entry_raises(self):
        spec = self._spec([
            {
                'object_name': 'a.tar.gz',
                'sha256sum': '1',
                'size_bytes': 1
            },
            {
                'object_name': 'b.tar.gz',
                'sha256sum': '2',
                'size_bytes': 2
            },
        ])
        with self.assertRaisesRegex(ValueError, 'single-object'):
            m.TarballInstaller.for_dep(Path('/ws'), 'src/p', spec)


class ForObjectTest(unittest.TestCase):
    """Tests for the `TarballInstaller.for_object` caller-supplied factory."""

    def test_builds_installer_from_a_caller_object(self):
        inst = m.TarballInstaller.for_object(Path('/dest'),
                                             'https://example.com/', {
                                                 'object_name': 'x.tar.gz',
                                                 'sha256sum': 'abc',
                                                 'size_bytes': 123,
                                             })
        self.assertEqual(inst.dest_dir, Path('/dest'))
        self.assertEqual(inst.url, 'https://example.com/x.tar.gz')
        self.assertEqual(inst.object_name, 'x.tar.gz')
        self.assertEqual(inst.sha256sum, 'abc')
        self.assertEqual(inst.size_bytes, 123)
        self.assertTrue(inst.owns_dest)

    def test_overlay_object_does_not_own_dest(self):
        inst = m.TarballInstaller.for_object(
            Path('/dest'), 'https://example.com/', {
                'object_name': 'x.tar.gz',
                'sha256sum': 'abc',
                'size_bytes': 123,
                'overlayed_on': 'base.tar.xz',
            })
        self.assertFalse(inst.owns_dest)


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
