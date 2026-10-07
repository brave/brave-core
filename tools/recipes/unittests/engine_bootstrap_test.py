#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the self-contained engine bootstrap."""

# White-box tests: they exercise bootstrap internals (`_deploy_recipes`,
# `_deploy_depot_tools`, `_run`), so protected-access is intentional.
# pylint: disable=protected-access

import os
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import engine_bootstrap as bootstrap  # pylint: disable=wrong-import-position

_HASH = 'c0ffee' * 6 + 'c0ff'


class MainForwardingTest(unittest.TestCase):
    """main() deploys the engine and launches it under vpython3, forwarding
    argv verbatim."""

    def test_forwards_argv_under_vpython(self):
        argv = ['toolchains/rust/package_rust', '--workspace', '/work']
        engine_path = Path(
            '/work/recipes_engine_bootstrap/tools/recipes/engine.py'
        )
        spec_path = engine_path.parent / bootstrap.VPYTHON_SPEC
        with (
            mock.patch.object(
                bootstrap, '_deploy_recipes', return_value=engine_path
            ) as deploy,
            mock.patch.object(
                bootstrap.shutil, 'which', return_value='/path/to/vpython3'
            ),
            mock.patch.object(bootstrap.subprocess, 'run') as run,
        ):
            run.return_value = types.SimpleNamespace(returncode=0)
            rc = bootstrap.main(argv)

        self.assertEqual(rc, 0)
        # The engine checkout is deployed under --workspace, and vpython3 was
        # on PATH, so the bare name is used (no depot_tools clone).
        deploy.assert_called_once_with(
            Path('/work') / bootstrap.RECIPES_ENGINE_DEST,
            bootstrap.BRAVE_CORE_REF,
        )
        forwarded = run.call_args[0][0]
        self.assertEqual(
            forwarded,
            [
                bootstrap.VPYTHON3,
                '-vpython-spec',
                str(spec_path),
                '-u',
                str(engine_path),
                *argv,
            ],
        )

    def test_missing_vpython_clones_depot_tools(self):
        """With no vpython3 on PATH, depot_tools is deployed under --workspace
        and its vpython3 (absolute path) is used to launch the engine."""
        engine_path = Path(
            '/work/recipes_engine_bootstrap/tools/recipes/engine.py'
        )
        depot_tools = Path('/work/depot_tools_bootstrap')
        with (
            mock.patch.dict(os.environ, clear=False),
            mock.patch.object(
                bootstrap, '_deploy_recipes', return_value=engine_path
            ),
            mock.patch.object(
                bootstrap, '_deploy_depot_tools', return_value=depot_tools
            ) as deploy_dt,
            mock.patch.object(bootstrap.shutil, 'which', return_value=None),
            mock.patch.object(bootstrap.subprocess, 'run') as run,
        ):
            run.return_value = types.SimpleNamespace(returncode=0)
            rc = bootstrap.main(
                ['toolchains/rust/package_rust', '--workspace', '/work']
            )
            # The cloned depot_tools is prepended to PATH (asserted inside the
            # patch.dict scope, which restores os.environ on exit) so the
            # engine and any recipe's depot_tools module reuse it.
            self.assertEqual(
                os.environ['PATH'].split(os.pathsep)[0], str(depot_tools)
            )

        self.assertEqual(rc, 0)
        # The clone destination is resolved relative to --workspace.
        deploy_dt.assert_called_once_with(
            Path('/work') / bootstrap.DEPOT_TOOLS_DEST
        )
        forwarded = run.call_args[0][0]
        self.assertEqual(forwarded[0], str(depot_tools / bootstrap.VPYTHON3))

    def test_revision_is_consumed(self):
        """--revision picks what is deployed and is not forwarded, as the
        engine does not know it."""
        engine_path = Path(
            '/work/recipes_engine_bootstrap/tools/recipes/engine.py'
        )
        with (
            mock.patch.object(
                bootstrap, '_deploy_recipes', return_value=engine_path
            ) as deploy,
            mock.patch.object(
                bootstrap.shutil, 'which', return_value='/path/to/vpython3'
            ),
            mock.patch.object(bootstrap.subprocess, 'run') as run,
        ):
            run.return_value = types.SimpleNamespace(returncode=0)
            bootstrap.main(
                [
                    'github/mirror_chromium',
                    '--revision',
                    _HASH,
                    '--workspace',
                    '/work',
                ]
            )

        deploy.assert_called_once_with(
            Path('/work') / bootstrap.RECIPES_ENGINE_DEST, _HASH
        )
        self.assertEqual(
            run.call_args[0][0][-3:],
            ['github/mirror_chromium', '--workspace', '/work'],
        )


class DeployRecipesTest(unittest.TestCase):
    """_deploy_recipes reuses, updates or clones the checkout at *dest*."""

    def _checkout(self, work, *, git=True):
        """A fake checkout under *work*, holding the engine and its spec."""
        dest = Path(work) / 'bc'
        recipes = dest / bootstrap.RECIPES_PATH
        recipes.mkdir(parents=True)
        for name in ('engine.py', bootstrap.VPYTHON_SPEC):
            (recipes / name).write_text('', encoding='utf-8', newline='')
        if git:
            (dest / '.git').mkdir()
        return dest.resolve()

    def _fetch_calls(self, dest, revision):
        return [
            mock.call(
                'git',
                '-C',
                dest,
                'fetch',
                '--depth',
                '2',
                '--filter=blob:none',
                'origin',
                revision,
            ),
            mock.call(
                'git',
                '-C',
                dest,
                'checkout',
                '--force',
                '--detach',
                'FETCH_HEAD',
            ),
        ]

    def _verify_call(self, dest):
        return mock.call(
            'git',
            '-C',
            dest,
            'rev-parse',
            '--verify',
            '--quiet',
            f'{_HASH}^{{commit}}',
            check=False,
        )

    def _diff_call(self, dest):
        return mock.call(
            'git', '-C', dest, 'diff', '--quiet', _HASH, '--', check=False
        )

    def _run_checks(self, *results):
        """A `_run` side effect: the `check=False` calls return *results* in
        order, the checked ones succeed."""
        results = iter(results)

        def run(*_cmd, cwd=None, check=True):  # pylint: disable=unused-argument
            return True if check else next(results)

        return run

    def test_reuses_checkout_at_revision(self):
        with tempfile.TemporaryDirectory() as work:
            dest = self._checkout(work)
            with mock.patch.object(
                bootstrap, '_run', side_effect=self._run_checks(True, True)
            ) as run:
                result = bootstrap._deploy_recipes(dest, _HASH)
        self.assertEqual(
            run.call_args_list, [self._verify_call(dest), self._diff_call(dest)]
        )
        self.assertEqual(result, dest / bootstrap.RECIPES_PATH / 'engine.py')

    def test_fetches_revision_not_in_checkout(self):
        with tempfile.TemporaryDirectory() as work:
            dest = self._checkout(work)
            with (
                mock.patch.object(
                    bootstrap, '_run', side_effect=self._run_checks(False)
                ) as run,
                mock.patch.object(bootstrap, '_rmtree') as rmtree,
            ):
                bootstrap._deploy_recipes(dest, _HASH)
        rmtree.assert_not_called()
        self.assertEqual(
            run.call_args_list,
            [self._verify_call(dest), *self._fetch_calls(dest, _HASH)],
        )

    def test_checks_out_revision_not_checked_out(self):
        with tempfile.TemporaryDirectory() as work:
            dest = self._checkout(work)
            with mock.patch.object(
                bootstrap, '_run', side_effect=self._run_checks(True, False)
            ) as run:
                bootstrap._deploy_recipes(dest, _HASH)
        self.assertEqual(
            run.call_args_list,
            [
                self._verify_call(dest),
                self._diff_call(dest),
                *self._fetch_calls(dest, _HASH),
            ],
        )

    def test_always_fetches_a_branch(self):
        """A branch may have moved, so it is never taken as checked out."""
        with tempfile.TemporaryDirectory() as work:
            dest = self._checkout(work)
            with mock.patch.object(bootstrap, '_run') as run:
                bootstrap._deploy_recipes(dest, 'master')
        self.assertEqual(run.call_args_list, self._fetch_calls(dest, 'master'))

    def test_clones_when_no_checkout(self):
        """Leftovers without a `.git` are cleared, then a sparse clone is
        made and *revision* fetched into it."""
        with tempfile.TemporaryDirectory() as work:
            # _rmtree is mocked, so the engine files survive to be found.
            dest = self._checkout(work, git=False)
            with (
                mock.patch.object(bootstrap, '_run') as run,
                mock.patch.object(bootstrap, '_rmtree') as rmtree,
            ):
                bootstrap._deploy_recipes(dest, _HASH)
        rmtree.assert_called_once_with(dest)
        self.assertEqual(
            run.call_args_list,
            [
                mock.call(
                    'git',
                    'clone',
                    '--depth',
                    '1',
                    '--filter=blob:none',
                    '--sparse',
                    '--no-checkout',
                    bootstrap.REPO_URL,
                    dest,
                ),
                mock.call(
                    'git',
                    '-C',
                    dest,
                    'sparse-checkout',
                    'add',
                    bootstrap.RECIPES_PATH,
                ),
                *self._fetch_calls(dest, _HASH),
            ],
        )

    def test_missing_engine_fails(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'bc'
            (dest / '.git').mkdir(parents=True)
            with (
                mock.patch.object(bootstrap, '_run'),
                self.assertRaisesRegex(RuntimeError, 'engine not found'),
            ):
                bootstrap._deploy_recipes(dest, 'master')


class DeployDepotToolsTest(unittest.TestCase):
    """_deploy_depot_tools reuses a good checkout and clones when absent."""

    def test_reuses_existing_vpython3(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'depot_tools'
            dest.mkdir()
            (dest / bootstrap.VPYTHON3).write_text(
                '', encoding='utf-8', newline=''
            )
            with mock.patch.object(bootstrap, '_run') as run:
                result = bootstrap._deploy_depot_tools(dest)
        run.assert_not_called()
        self.assertEqual(result, dest.resolve())

    def _fake_clone(self, dest):
        """A `_run` side effect simulating the clone producing a vpython3."""

        def fake_clone(*_cmd, cwd=None):  # pylint: disable=unused-argument
            dest.mkdir(parents=True, exist_ok=True)
            (dest / bootstrap.VPYTHON3).write_text(
                '', encoding='utf-8', newline=''
            )

        return fake_clone

    def test_clones_when_absent(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'depot_tools'
            with (
                mock.patch.dict(os.environ),
                mock.patch.object(
                    bootstrap, '_run', side_effect=self._fake_clone(dest)
                ) as run,
            ):
                os.environ.pop('GIT_CACHE_PATH', None)
                result = bootstrap._deploy_depot_tools(dest)
        run.assert_called_once_with(
            'git',
            'clone',
            '--depth',
            '1',
            bootstrap.DEPOT_TOOLS_URL,
            dest.resolve(),
        )
        self.assertEqual(result, dest.resolve())

    def test_clones_remotely_when_cache_has_no_mirror(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'depot_tools'
            cache = Path(work) / 'cache'
            cache.mkdir()
            with (
                mock.patch.dict(os.environ, {'GIT_CACHE_PATH': str(cache)}),
                mock.patch.object(
                    bootstrap, '_run', side_effect=self._fake_clone(dest)
                ) as run,
            ):
                bootstrap._deploy_depot_tools(dest)
        run.assert_called_once_with(
            'git',
            'clone',
            '--depth',
            '1',
            bootstrap.DEPOT_TOOLS_URL,
            dest.resolve(),
        )

    def test_shared_clones_from_cache_mirror(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'depot_tools'
            cache = Path(work) / 'cache'
            mirror = cache / bootstrap.DEPOT_TOOLS_MIRROR_DIR
            mirror.mkdir(parents=True)
            (mirror / 'config').write_text('', encoding='utf-8', newline='')
            with (
                mock.patch.dict(os.environ, {'GIT_CACHE_PATH': str(cache)}),
                mock.patch.object(
                    bootstrap, '_run', side_effect=self._fake_clone(dest)
                ) as run,
            ):
                result = bootstrap._deploy_depot_tools(dest)
        self.assertEqual(
            run.call_args_list,
            [
                mock.call('git', 'clone', '--shared', mirror, dest.resolve()),
                mock.call(
                    'git',
                    '-C',
                    dest.resolve(),
                    'remote',
                    'set-url',
                    'origin',
                    bootstrap.DEPOT_TOOLS_URL,
                ),
            ],
        )
        self.assertEqual(result, dest.resolve())


if __name__ == '__main__':
    unittest.main()
