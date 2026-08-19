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


class ProjectRefTest(unittest.TestCase):
    """Mirrors `Config.getProjectRef()` in build/commands/lib/config.ts."""

    def test_revision_wins(self):
        self.assertEqual(
            fetch_brave.project_ref({
                'revision': 'deadbeef',
                'tag': '1.2.3.4',
                'branch': 'release',
            }), 'deadbeef')

    def test_tag_becomes_a_tag_ref(self):
        self.assertEqual(
            fetch_brave.project_ref({
                'tag': '152.0.7977.42',
                'branch': 'release',
            }), 'refs/tags/152.0.7977.42')

    def test_branch_becomes_an_origin_ref(self):
        self.assertEqual(fetch_brave.project_ref({'branch': 'release'}),
                         'origin/release')

    def test_defaults_to_origin_master(self):
        self.assertEqual(fetch_brave.project_ref({}), 'origin/master')


class BuildSolutionsTest(unittest.TestCase):
    """Exercises the permanent `.gclient` solutions list shape."""

    def test_both_solutions_are_unmanaged(self):
        solutions = fetch_brave.build_solutions(
            {
                'dir': 'src',
                'repository': {
                    'url': 'https://example.test/chromium/src.git'
                },
                'custom_deps': {
                    'src/foo': None
                },
                'custom_vars': {
                    'bar': True
                },
            }, 'https://example.test/brave-core.git')

        self.assertEqual(len(solutions), 2)
        self.assertEqual(
            solutions[0], {
                'managed': False,
                'name': 'src',
                'url': 'https://example.test/chromium/src.git',
                'custom_deps': {
                    'src/foo': None
                },
                'custom_vars': {
                    'bar': True
                },
            })
        self.assertEqual(
            solutions[1], {
                'managed': False,
                'name': 'src/brave',
                'url': 'https://example.test/brave-core.git',
            })

    def test_missing_custom_deps_and_vars_default_to_empty(self):
        solutions = fetch_brave.build_solutions(
            {'repository': {
                'url': 'https://example.test/src.git'
            }}, 'https://example.test/brave-core.git')
        self.assertEqual(solutions[0]['custom_deps'], {})
        self.assertEqual(solutions[0]['custom_vars'], {})


class RenderGclientTest(unittest.TestCase):
    """Checks the generated `.gclient` text is valid, gclient-parseable
    Python, and matches the real file's inline/wrapped layout.
    """

    def _parse(self, text: str) -> dict:
        out: dict = {}
        exec(compile(text, '.gclient', 'exec'), None, out)  # pylint: disable=exec-used
        return out

    def test_round_trips_through_exec(self):
        solutions = fetch_brave.build_solutions(
            {
                'repository': {
                    'url': 'https://example.test/src.git'
                },
                'custom_deps': {
                    'src/foo': None
                },
                'custom_vars': {
                    'bar': True,
                    'baz': False
                },
            }, 'https://example.test/brave-core.git')
        text = fetch_brave.render_gclient(solutions, Path('/tmp/git-cache'),
                                          [], [])
        parsed = self._parse(text)

        self.assertEqual(parsed['solutions'], solutions)
        self.assertEqual(parsed['cache_dir'], '/tmp/git-cache')
        self.assertEqual(parsed['target_os'], [])
        self.assertEqual(parsed['target_cpu'], [])

    def test_short_values_are_inlined_long_ones_are_wrapped(self):
        solutions = fetch_brave.build_solutions(
            {'repository': {
                'url': 'https://example.test/src.git'
            }}, 'https://example.test/brave-core.git')
        text = fetch_brave.render_gclient(solutions, Path('/c'), [], ['x64'])
        # `target_cpu = ["x64"]` fits in 80 columns, so it stays inline.
        self.assertIn('target_os = []\n', text)
        self.assertIn('target_cpu = ["x64"]\n', text)
        # The `solutions` entry does not, so it wraps one item per line.
        self.assertIn('solutions = [\n  {\n', text)


class CloneIfMissingTest(unittest.TestCase):
    """Exercises the skip-if-already-a-checkout behaviour."""

    def test_clones_when_absent(self):
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp) / 'nested' / 'depot_tools'
            with mock.patch('subprocess.check_call') as check_call:
                cloned = fetch_brave.clone_if_missing('https://example.test/x',
                                                      dest)
            self.assertTrue(cloned)
            check_call.assert_called_once_with(
                ['git', 'clone', 'https://example.test/x',
                 str(dest)])
            # The parent directory is created so git has somewhere to clone
            # into, even though the leaf itself is left for git to create.
            self.assertTrue(dest.parent.is_dir())

    def test_skips_when_dot_git_already_present(self):
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp) / 'depot_tools'
            (dest / '.git').mkdir(parents=True)
            with mock.patch('subprocess.check_call') as check_call:
                cloned = fetch_brave.clone_if_missing('https://example.test/x',
                                                      dest)
            self.assertFalse(cloned)
            check_call.assert_not_called()


class ResolveBootstrapDepotToolsTest(unittest.TestCase):
    """Exercises the on-$PATH-vs-clone-fresh decision."""

    def test_reuses_gclient_already_on_path(self):
        with mock.patch('shutil.which', return_value='/usr/bin/gclient'), \
             mock.patch('subprocess.check_call') as check_call:
            depot_tools_dir, freshly_cloned = (
                fetch_brave.resolve_bootstrap_depot_tools(Path('/unused')))
        self.assertEqual(depot_tools_dir, Path('/usr/bin'))
        self.assertFalse(freshly_cloned)
        check_call.assert_not_called()

    def test_clones_when_nothing_on_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            scratch_dir = Path(tmp) / 'depot_tools'
            with mock.patch('shutil.which', return_value=None), \
                 mock.patch('subprocess.check_call') as check_call:
                depot_tools_dir, freshly_cloned = (
                    fetch_brave.resolve_bootstrap_depot_tools(scratch_dir))
            self.assertEqual(depot_tools_dir, scratch_dir)
            self.assertTrue(freshly_cloned)
            check_call.assert_called_once_with([
                'git', 'clone', fetch_brave.DEPOT_TOOLS_URL,
                str(scratch_dir)
            ])


class CloneBraveCoreTest(unittest.TestCase):
    """Exercises the bootstrap (brave-core-only) `gclient sync` pass."""

    def test_writes_scratch_gclient_and_syncs_with_revision(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with mock.patch('fetch_brave.run_gclient') as run_gclient:
                fetch_brave.clone_brave_core(Path('/depot_tools'), root,
                                             'https://example.test/brave.git',
                                             'release', Path('/cache'))

            gclient_text = (root / '.gclient').read_text(encoding='utf-8')
            self.assertIn('"name": "src/brave"', gclient_text)
            self.assertIn('"url": "https://example.test/brave.git"',
                          gclient_text)
            run_gclient.assert_called_once_with(
                Path('/depot_tools'), root,
                ['sync', '--nohooks', '--revision', 'src/brave@release'])

    def test_omits_revision_when_no_ref_given(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with mock.patch('fetch_brave.run_gclient') as run_gclient:
                fetch_brave.clone_brave_core(Path('/depot_tools'), root,
                                             'https://example.test/brave.git',
                                             None, Path('/cache'))
            run_gclient.assert_called_once_with(Path('/depot_tools'), root,
                                                ['sync', '--nohooks'])


class VendorDepotToolsTest(unittest.TestCase):
    """Exercises the move-if-fresh / clone-if-reused vendoring decision."""

    def test_moves_a_freshly_cloned_scratch_checkout_into_place(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            scratch = root / 'scratch_depot_tools'
            (scratch / '.git').mkdir(parents=True)
            (scratch / 'gclient.py').write_text('# stub', encoding='utf-8')
            brave_core_dir = root / 'src' / 'brave'
            brave_core_dir.mkdir(parents=True)

            with mock.patch('subprocess.check_call') as check_call:
                vendor_dir = fetch_brave.vendor_depot_tools(
                    scratch, True, brave_core_dir)

            check_call.assert_not_called()
            self.assertEqual(vendor_dir,
                             brave_core_dir / 'vendor' / 'depot_tools')
            self.assertTrue((vendor_dir / 'gclient.py').is_file())
            self.assertFalse(scratch.exists())

    def test_clones_a_fresh_copy_when_reusing_a_path_depot_tools(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path_depot_tools = root / 'usr_bin_depot_tools'
            path_depot_tools.mkdir()  # stands in for a real $PATH depot_tools
            brave_core_dir = root / 'src' / 'brave'
            brave_core_dir.mkdir(parents=True)

            with mock.patch('subprocess.check_call') as check_call:
                vendor_dir = fetch_brave.vendor_depot_tools(
                    path_depot_tools, False, brave_core_dir)

            check_call.assert_called_once_with(
                ['git', 'clone', fetch_brave.DEPOT_TOOLS_URL,
                 str(vendor_dir)])
            self.assertTrue(path_depot_tools.exists())  # left untouched

    def test_skips_when_already_vendored(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            brave_core_dir = root / 'src' / 'brave'
            vendor_dir = brave_core_dir / 'vendor' / 'depot_tools'
            (vendor_dir / '.git').mkdir(parents=True)

            with mock.patch('subprocess.check_call') as check_call:
                result = fetch_brave.vendor_depot_tools(
                    Path('/whatever'), True, brave_core_dir)

            check_call.assert_not_called()
            self.assertEqual(result, vendor_dir)


class EnsureBootstrapOnPathTest(unittest.TestCase):
    """Exercises the prepend-unless-already-present PATH logic."""

    BOOTSTRAP_DIR = Path(fetch_brave.__file__).resolve().parent

    def test_prepends_when_absent(self):
        env = fetch_brave.ensure_bootstrap_on_path({'PATH': '/usr/bin'})
        self.assertEqual(env['PATH'],
                         f'{self.BOOTSTRAP_DIR}{os.pathsep}/usr/bin')

    def test_does_not_duplicate_when_already_present(self):
        original = f'/usr/bin{os.pathsep}{self.BOOTSTRAP_DIR}'
        env = fetch_brave.ensure_bootstrap_on_path({'PATH': original})
        self.assertEqual(env['PATH'], original)

    def test_recognises_a_non_normalised_equivalent_path(self):
        noisy = str(self.BOOTSTRAP_DIR / '..' / self.BOOTSTRAP_DIR.name)
        env = fetch_brave.ensure_bootstrap_on_path({'PATH': noisy})
        self.assertEqual(env['PATH'], noisy)

    def test_leaves_the_input_env_untouched(self):
        original_env = {'PATH': '/usr/bin'}
        fetch_brave.ensure_bootstrap_on_path(original_env)
        self.assertEqual(original_env, {'PATH': '/usr/bin'})


class RunPnpmTest(unittest.TestCase):
    """Exercises the `pnpm` invocation: cwd, args, and the PATH prepend."""

    def test_runs_pnpm_with_bootstrap_on_path(self):
        brave_core_dir = Path('/checkout/src/brave')
        with mock.patch.dict('os.environ', {'PATH': '/usr/bin'}, clear=True), \
             mock.patch('subprocess.check_call') as check_call:
            fetch_brave.run_pnpm(brave_core_dir,
                                 ['install', '--frozen-lockfile'])

        check_call.assert_called_once()
        called_args, called_kwargs = check_call.call_args
        self.assertEqual(called_args[0],
                         ['pnpm', 'install', '--frozen-lockfile'])
        self.assertEqual(called_kwargs['cwd'], brave_core_dir)
        bootstrap_dir = Path(fetch_brave.__file__).resolve().parent
        self.assertEqual(called_kwargs['env']['PATH'],
                         f'{bootstrap_dir}{os.pathsep}/usr/bin')


class DefaultCacheDirTest(unittest.TestCase):
    """Exercises the $GIT_CACHE_PATH override and its fallback."""

    def test_honours_git_cache_path_env_var(self):
        with mock.patch.dict('os.environ', {'GIT_CACHE_PATH': '/configured'}):
            self.assertEqual(fetch_brave.default_cache_dir(),
                             Path('/configured'))

    def test_falls_back_to_a_shared_location(self):
        with mock.patch.dict('os.environ', {}, clear=True):
            self.assertEqual(fetch_brave.default_cache_dir(),
                             Path.home() / '.cache' / 'brave' / 'git-cache')


if __name__ == '__main__':
    unittest.main()
