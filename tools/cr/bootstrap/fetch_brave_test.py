#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Unit tests for tools/cr/bootstrap/fetch_brave.py."""

from __future__ import annotations

import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import fetch_brave


class EnsureDepotToolsTest(unittest.TestCase):
    """Exercises vendoring depot_tools -- the only place it ever lives.

    The mirror lookup deliberately never shells out to `git cache exists`
    (depot_tools' own subcommand): it's checked before any depot_tools is
    available to run that with, directly against the hardcoded
    `_DEPOT_TOOLS_MIRROR_DIR` name instead.
    """

    def test_plain_clones_when_no_cache_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            brave_core_dir = Path(tmp) / 'src' / 'brave'
            brave_core_dir.mkdir(parents=True)

            with mock.patch('subprocess.run') as run, \
                 mock.patch('subprocess.check_call') as check_call:
                dest = fetch_brave.ensure_depot_tools(brave_core_dir, None)

            run.assert_not_called()  # never shells out to `git cache exists`
            self.assertEqual(dest, brave_core_dir / 'vendor' / 'depot_tools')
            check_call.assert_called_once_with(
                ['git', 'clone', fetch_brave.DEPOT_TOOLS_URL,
                 str(dest)])

    def test_plain_clones_when_cache_dir_has_no_depot_tools_mirror(self):
        with tempfile.TemporaryDirectory() as tmp:
            brave_core_dir = Path(tmp) / 'src' / 'brave'
            brave_core_dir.mkdir(parents=True)
            cache_dir = Path(tmp) / 'cache'  # exists, but no mirror inside it
            cache_dir.mkdir()

            with mock.patch('subprocess.run') as run, \
                 mock.patch('subprocess.check_call') as check_call:
                dest = fetch_brave.ensure_depot_tools(brave_core_dir,
                                                      cache_dir)

            run.assert_not_called()
            check_call.assert_called_once_with(
                ['git', 'clone', fetch_brave.DEPOT_TOOLS_URL,
                 str(dest)])

    def test_reuses_a_cached_mirror(self):
        with tempfile.TemporaryDirectory() as tmp:
            brave_core_dir = Path(tmp) / 'src' / 'brave'
            brave_core_dir.mkdir(parents=True)
            cache_dir = Path(tmp) / 'cache'
            cache_repo_dir = cache_dir / fetch_brave._DEPOT_TOOLS_MIRROR_DIR  # pylint: disable=protected-access
            cache_repo_dir.mkdir(parents=True)
            (cache_repo_dir / 'config').write_text('', encoding='utf-8')

            with mock.patch('subprocess.check_call') as check_call:
                dest = fetch_brave.ensure_depot_tools(brave_core_dir,
                                                      cache_dir)

            self.assertEqual(check_call.call_args_list, [
                mock.call([
                    'git', 'clone', '--no-checkout', '--local', '--shared',
                    str(cache_repo_dir),
                    str(dest)
                ]),
                mock.call(['git', 'checkout', '--force', 'HEAD'], cwd=dest),
                mock.call([
                    'git', 'remote', 'set-url', 'origin',
                    fetch_brave.DEPOT_TOOLS_URL
                ],
                          cwd=dest),
            ])

    def test_skips_when_already_vendored(self):
        with tempfile.TemporaryDirectory() as tmp:
            brave_core_dir = Path(tmp) / 'src' / 'brave'
            dest = brave_core_dir / 'vendor' / 'depot_tools'
            (dest / '.git').mkdir(parents=True)

            with mock.patch('subprocess.check_call') as check_call:
                result = fetch_brave.ensure_depot_tools(
                    brave_core_dir, Path('/cache'))

            check_call.assert_not_called()
            self.assertEqual(result, dest)


class FetchBraveCoreTest(unittest.TestCase):
    """Exercises the bootstrap (brave-core-only) `gclient sync` pass."""

    def _parse_gclient(self, root: Path) -> dict:
        text = (root / '.gclient').read_text(encoding='utf-8')
        out: dict = {}
        exec(compile(text, '.gclient', 'exec'), None, out)  # pylint: disable=exec-used
        return out

    def test_writes_a_valid_gclient_and_syncs_with_revision(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with mock.patch('subprocess.check_call') as check_call:
                fetch_brave.fetch_brave_core(Path('/depot_tools'), root,
                                             'release')

            parsed = self._parse_gclient(root)
            self.assertEqual(parsed['solutions'], [{
                'managed': False,
                'name': 'src/brave',
                'url': fetch_brave.BRAVE_CORE_URL,
            }])
            self.assertEqual(parsed['target_os'], [])
            self.assertEqual(parsed['target_cpu'], [])
            # No `cache_dir`: `main()` sets `$GIT_CACHE_PATH` instead, which
            # depot_tools already falls back to on its own.
            self.assertNotIn('cache_dir', parsed)
            check_call.assert_called_once_with([
                f'/depot_tools/{fetch_brave.GCLIENT_NAME}', 'sync',
                '--nohooks', '--revision', 'src/brave@release'
            ],
                                               cwd=root)

    def test_omits_revision_when_no_ref_given(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with mock.patch('subprocess.check_call') as check_call:
                fetch_brave.fetch_brave_core(Path('/depot_tools'), root, None)
            check_call.assert_called_once_with([
                f'/depot_tools/{fetch_brave.GCLIENT_NAME}', 'sync', '--nohooks'
            ],
                                               cwd=root)

    def test_uses_the_bat_wrapper_on_windows(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with mock.patch('fetch_brave.GCLIENT_NAME', 'gclient.bat'), \
                 mock.patch('subprocess.check_call') as check_call:
                fetch_brave.fetch_brave_core(Path('/depot_tools'), root, None)
            check_call.assert_called_once_with(
                [str(Path('/depot_tools/gclient.bat')), 'sync', '--nohooks'],
                cwd=root)


class PrependToPathTest(unittest.TestCase):
    """Exercises prepending dirs to a `$PATH` string."""

    def test_prepends_dirs_in_order(self):
        result = fetch_brave.prepend_to_path('/usr/bin', Path('/a'),
                                             Path('/b'))
        self.assertEqual(result, f'/a{os.pathsep}/b{os.pathsep}/usr/bin')

    def test_handles_an_empty_path(self):
        result = fetch_brave.prepend_to_path('', Path('/a'), Path('/b'))
        self.assertEqual(result, f'/a{os.pathsep}/b')

    def test_leaves_an_odd_existing_path_untouched(self):
        # A leading `:` means "include the current directory" on POSIX --
        # rebuilding the string from parsed entries would silently drop it.
        odd = f':/a{os.pathsep}/usr/bin'
        result = fetch_brave.prepend_to_path(odd, Path('/new'))
        self.assertEqual(result, f'/new{os.pathsep}{odd}')


class RunPnpmTest(unittest.TestCase):
    """Exercises the `pnpm` invocation: cwd and args."""

    def test_runs_pnpm(self):
        brave_core_dir = Path('/checkout/src/brave')
        with mock.patch('subprocess.check_call') as check_call:
            fetch_brave.run_pnpm(brave_core_dir,
                                 ['install', '--frozen-lockfile'])
        check_call.assert_called_once_with(
            ['pnpm', 'install', '--frozen-lockfile'], cwd=brave_core_dir)


if __name__ == '__main__':
    unittest.main()
