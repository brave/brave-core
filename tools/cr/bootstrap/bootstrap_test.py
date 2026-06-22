#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Unit tests for the tools/cr/bootstrap pure helpers."""

from __future__ import annotations

import os
from pathlib import Path
import platform
import stat
import subprocess
import tempfile
import unittest

import bootstrap
import launcher


class PosixBlockTest(unittest.TestCase):
    DIR = Path('/home/dev/src/brave/tools/cr/bootstrap')

    def test_apply_block_appends_single_block(self):
        result = bootstrap.apply_block('existing line\n', self.DIR)
        self.assertEqual(result.count(bootstrap.BEGIN_MARKER), 1)
        self.assertEqual(result.count(bootstrap.END_MARKER), 1)
        self.assertIn('existing line\n', result)
        self.assertIn(f'export PATH="{self.DIR.as_posix()}:$PATH"', result)

    def test_apply_block_is_idempotent(self):
        once = bootstrap.apply_block('existing line\n', self.DIR)
        twice = bootstrap.apply_block(once, self.DIR)
        self.assertEqual(once, twice)
        self.assertEqual(twice.count(bootstrap.BEGIN_MARKER), 1)

    def test_apply_block_repoints_on_new_dir(self):
        first = bootstrap.apply_block('', Path('/old/bootstrap'))
        second = bootstrap.apply_block(first, Path('/new/bootstrap'))
        self.assertEqual(second.count(bootstrap.BEGIN_MARKER), 1)
        self.assertNotIn('/old/bootstrap', second)
        self.assertIn('/new/bootstrap', second)

    def test_remove_block_round_trips(self):
        base = 'line one\nline two\n'
        self.assertEqual(
            bootstrap.remove_block(bootstrap.apply_block(base, self.DIR)),
            base)

    def test_remove_block_no_op_without_block(self):
        text = 'nothing to see here\n'
        self.assertEqual(bootstrap.remove_block(text), text)

    def test_apply_block_on_empty(self):
        result = bootstrap.apply_block('', self.DIR)
        self.assertTrue(result.startswith(bootstrap.BEGIN_MARKER))
        self.assertTrue(result.endswith('\n'))


class FishDropInTest(unittest.TestCase):

    def test_prepends_path_without_fish_add_path(self):
        content = bootstrap.fish_drop_in(Path('/a/b/bootstrap'))
        # Prepends to $PATH and drops any prior occurrence (lands first, once).
        self.assertIn(
            'set -gx PATH "/a/b/bootstrap" '
            '(string match --invert -- "/a/b/bootstrap" $PATH)', content)
        # Must NOT use fish_add_path: it persists universal state that would
        # survive uninstall (deleting this file).
        self.assertNotIn('fish_add_path', content)
        self.assertIn('Managed by', content)


class WindowsPathTest(unittest.TestCase):
    DIR = Path(r'C:\dev\src\brave\tools\cr\bootstrap')

    def test_add_prepends_first(self):
        result = bootstrap.add_windows_entry(r'C:\Windows;C:\Tools', self.DIR)
        self.assertEqual(result, rf'{self.DIR};C:\Windows;C:\Tools')

    def test_add_moves_existing_to_front(self):
        # Present (lowercased, trailing slash) but not first -> moved to front
        # exactly once, in canonical form.
        current = rf'C:\Windows;{str(self.DIR).lower()}\\;C:\Tools'
        self.assertEqual(bootstrap.add_windows_entry(current, self.DIR),
                         rf'{self.DIR};C:\Windows;C:\Tools')

    def test_add_is_idempotent(self):
        once = bootstrap.add_windows_entry(r'C:\Windows', self.DIR)
        self.assertEqual(bootstrap.add_windows_entry(once, self.DIR), once)

    def test_add_to_empty(self):
        self.assertEqual(bootstrap.add_windows_entry('', self.DIR),
                         str(self.DIR))

    def test_remove_drops_entry(self):
        current = rf'C:\Windows;{self.DIR};C:\Tools'
        self.assertEqual(bootstrap.remove_windows_entry(current, self.DIR),
                         r'C:\Windows;C:\Tools')

    def test_remove_no_op(self):
        current = r'C:\Windows;C:\Tools'
        self.assertEqual(bootstrap.remove_windows_entry(current, self.DIR),
                         current)


class FindExistingBootstrapTest(unittest.TestCase):

    def setUp(self):
        self._saved_path = os.environ.get('PATH', '')

    def tearDown(self):
        os.environ['PATH'] = self._saved_path

    def test_none_when_not_on_path(self):
        os.environ['PATH'] = ''
        self.assertIsNone(bootstrap.find_existing_bootstrap())

    @unittest.skipIf(platform.system() == 'Windows',
                     'POSIX exec-bit lookup; Windows uses PATHEXT')
    def test_found_when_shim_on_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            shim_path = Path(tmp) / 'brockit'
            shim_path.write_text('#!/bin/sh\n', encoding='utf-8', newline='')
            shim_path.chmod(shim_path.stat().st_mode | stat.S_IXUSR)
            os.environ['PATH'] = tmp
            self.assertEqual(bootstrap.find_existing_bootstrap(),
                             str(shim_path))

    def test_installed_dir_falls_back_to_own_dir(self):
        os.environ['PATH'] = ''
        self.assertEqual(bootstrap.installed_bootstrap_dir(),
                         bootstrap.BOOTSTRAP_DIR)

    @unittest.skipIf(platform.system() == 'Windows',
                     'POSIX exec-bit lookup; Windows uses PATHEXT')
    def test_installed_dir_is_parent_of_on_path_shim(self):
        # uninstall must target wherever the live shim actually resides, which
        # may differ from this checkout's BOOTSTRAP_DIR.
        with tempfile.TemporaryDirectory() as tmp:
            other = Path(tmp).resolve() / 'other-checkout'
            other.mkdir()
            shim_path = other / 'plaster'
            shim_path.write_text('#!/bin/sh\n', encoding='utf-8', newline='')
            shim_path.chmod(shim_path.stat().st_mode | stat.S_IXUSR)
            os.environ['PATH'] = str(other)
            self.assertEqual(bootstrap.installed_bootstrap_dir(), other)


class FindBraveCoreTest(unittest.TestCase):
    """Exercises `launcher.find_brave_core` against real git work trees.

    `find_brave_core` resolves the checkout from the current directory via
    `git rev-parse`, so these tests build a throwaway repo and chdir into it
    rather than mocking git.
    """

    def setUp(self):
        self._cwd = Path.cwd()

    def tearDown(self):
        os.chdir(self._cwd)

    def _make_brave_repo(self,
                         root: Path,
                         *,
                         name: str = 'brave',
                         tool: str | None = 'brockit') -> Path:
        repo = root / name
        (repo / 'tools' / 'cr').mkdir(parents=True)
        if tool:
            (repo / 'tools' / 'cr' / f'{tool}.py').write_text('#',
                                                              encoding='utf-8',
                                                              newline='')
        subprocess.run(['git', 'init', '-q', str(repo)], check=True)
        return repo

    _BROCKIT = launcher.TOOL_PATHS['brockit']
    _PLASTER = launcher.TOOL_PATHS['plaster']

    def test_valid_brave_checkout(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = self._make_brave_repo(Path(tmp))
            os.chdir(repo / 'tools' / 'cr')  # any subdir of the work tree
            self.assertEqual(
                launcher.find_brave_core(self._BROCKIT).resolve(),
                repo.resolve())

    def test_rejects_non_brave_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = self._make_brave_repo(Path(tmp), name='notbrave')
            os.chdir(repo)
            self.assertIsNone(launcher.find_brave_core(self._BROCKIT))

    def test_rejects_missing_tool_script(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = self._make_brave_repo(Path(tmp), tool='brockit')
            os.chdir(repo)
            self.assertIsNone(launcher.find_brave_core(self._PLASTER))

    def test_rejects_outside_git_work_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            os.chdir(tmp)  # not a git repo
            self.assertIsNone(launcher.find_brave_core(self._BROCKIT))


class ResolveVpython3Test(unittest.TestCase):

    def test_resolves_to_a_vpython3_interpreter(self):
        # pylint: disable=protected-access
        # Whether found on $PATH or via the chromium-bundled fallback, the
        # resolved interpreter is always a vpython3 (vpython3.bat on Windows).
        resolved = launcher._resolve_vpython3(Path('/home/dev/src/brave'))
        self.assertIn(resolved.name, ('vpython3', 'vpython3.bat'))


if __name__ == '__main__':
    unittest.main()
