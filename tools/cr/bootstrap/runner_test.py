#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Unit tests for tools/cr/bootstrap/runner.py."""

from __future__ import annotations

import contextlib
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import runner


def _fake_clone(*cmd):
    """A `_run` side effect that makes `git clone` produce a vpython3."""
    if cmd[1] == 'clone':
        dest = Path(cmd[-1])
        dest.mkdir(parents=True)
        (dest / runner.VPYTHON3).write_text('', encoding='utf-8')


class EnsureDepotToolsTest(unittest.TestCase):
    """Exercises deploying `vendor/depot_tools` into a checkout."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.root = Path(self._tmp.name)
        self.checkout = self.root / 'src' / 'brave'
        self.checkout.mkdir(parents=True)
        self.dest = self.checkout / 'vendor' / 'depot_tools'
        self.guard = self.dest.parent / 'depot_tools_install.guard'
        env = mock.patch.dict(os.environ)
        env.start()
        self.addCleanup(env.stop)
        os.environ.pop('GIT_CACHE_PATH', None)
        stderr = mock.patch('sys.stderr')
        stderr.start()
        self.addCleanup(stderr.stop)

    def _make_mirror(self) -> Path:
        cache = self.root / 'cache'
        mirror = cache / runner._DEPOT_TOOLS_MIRROR_DIR  # pylint: disable=protected-access
        mirror.mkdir(parents=True)
        (mirror / 'config').write_text('', encoding='utf-8')
        os.environ['GIT_CACHE_PATH'] = str(cache)
        return mirror

    def _make_vpython3(self):
        self.dest.mkdir(parents=True)
        (self.dest / runner.VPYTHON3).write_text('', encoding='utf-8')

    def test_reuses_an_existing_checkout(self):
        self._make_vpython3()
        with mock.patch.object(runner, '_run') as run:
            self.assertEqual(runner.ensure_depot_tools(self.checkout),
                             self.dest)
        run.assert_not_called()

    def test_clones_from_the_web_without_git_cache(self):
        with mock.patch.object(runner, '_run', side_effect=_fake_clone) as run:
            self.assertEqual(runner.ensure_depot_tools(self.checkout),
                             self.dest)
        run.assert_called_once_with('git', 'clone', runner.DEPOT_TOOLS_URL,
                                    self.dest)
        self.assertFalse(self.guard.exists())

    def test_clones_from_the_web_when_cache_has_no_mirror(self):
        os.environ['GIT_CACHE_PATH'] = str(self.root / 'empty_cache')
        with mock.patch.object(runner, '_run', side_effect=_fake_clone) as run:
            runner.ensure_depot_tools(self.checkout)
        run.assert_called_once_with('git', 'clone', runner.DEPOT_TOOLS_URL,
                                    self.dest)

    def test_shared_clones_from_the_cache_mirror(self):
        mirror = self._make_mirror()
        with mock.patch.object(runner, '_run', side_effect=_fake_clone) as run:
            self.assertEqual(runner.ensure_depot_tools(self.checkout),
                             self.dest)
        self.assertEqual(run.call_args_list, [
            mock.call('git', 'clone', '--shared', mirror, self.dest),
            mock.call('git', '-C', self.dest, 'remote', 'set-url', 'origin',
                      runner.DEPOT_TOOLS_URL),
        ])
        self.assertFalse(self.guard.exists())

    def test_guards_the_clone_while_it_runs(self):

        def assert_guarded(*cmd):
            self.assertTrue(self.guard.is_file())
            _fake_clone(*cmd)

        with mock.patch.object(runner, '_run', side_effect=assert_guarded):
            runner.ensure_depot_tools(self.checkout)
        self.assertFalse(self.guard.exists())

    def test_failed_clone_leaves_the_guard_for_init(self):
        with mock.patch.object(runner,
                               '_run',
                               side_effect=subprocess.CalledProcessError(
                                   128, 'git')):
            self.assertIsNone(runner.ensure_depot_tools(self.checkout))
        self.assertTrue(self.guard.is_file())

    def test_interrupted_clone_leaves_the_guard_for_init(self):
        with mock.patch.object(runner, '_run',
                               side_effect=KeyboardInterrupt), \
             self.assertRaises(KeyboardInterrupt):
            runner.ensure_depot_tools(self.checkout)
        self.assertTrue(self.guard.is_file())

    def test_redeploys_a_guarded_checkout(self):
        # Even one with a vpython3, as the deploy may not have finished.
        self._make_vpython3()
        (self.dest / 'stale').write_text('', encoding='utf-8')
        self.guard.write_text('', encoding='utf-8')
        with mock.patch.object(runner, '_run', side_effect=_fake_clone) as run:
            self.assertEqual(runner.ensure_depot_tools(self.checkout),
                             self.dest)
        run.assert_called_once_with('git', 'clone', runner.DEPOT_TOOLS_URL,
                                    self.dest)
        self.assertFalse((self.dest / 'stale').exists())
        self.assertFalse(self.guard.exists())

    def test_redeploys_when_guarded_without_a_checkout(self):
        self.dest.parent.mkdir(parents=True)
        self.guard.write_text('', encoding='utf-8')
        with mock.patch.object(runner, '_run', side_effect=_fake_clone):
            self.assertEqual(runner.ensure_depot_tools(self.checkout),
                             self.dest)
        self.assertFalse(self.guard.exists())

    def test_leaves_a_broken_checkout_alone(self):
        self.dest.mkdir(parents=True)
        with mock.patch.object(runner, '_run') as run:
            self.assertIsNone(runner.ensure_depot_tools(self.checkout))
        run.assert_not_called()
        self.assertTrue(self.dest.is_dir())


class MainTest(unittest.TestCase):
    """Exercises which `vpython3` is printed for the shims."""

    def _main(self, *, on_path=None, checkout=None, depot_tools=None):
        out = io.StringIO()
        with mock.patch('shutil.which', return_value=on_path), \
             mock.patch.object(runner.launcher, '_resolve_checkout',
                               return_value=checkout), \
             mock.patch.object(runner, 'ensure_depot_tools',
                               return_value=depot_tools) as ensure, \
             contextlib.redirect_stdout(out):
            rc = runner.main()
        return rc, ensure, out.getvalue()

    def test_prints_vpython3_on_path_without_deploying(self):
        rc, ensure, out = self._main(on_path='/depot_tools/vpython3')
        self.assertEqual(rc, 0)
        ensure.assert_not_called()
        self.assertEqual(out, '/depot_tools/vpython3\n')

    def test_prints_the_governing_checkouts_vpython3(self):
        checkout = Path('/w/src/brave')
        depot_tools = checkout / 'vendor' / 'depot_tools'
        rc, ensure, out = self._main(checkout=checkout,
                                     depot_tools=depot_tools)
        self.assertEqual(rc, 0)
        ensure.assert_called_once_with(checkout)
        self.assertEqual(out, f'{depot_tools / runner.VPYTHON3}\n')

    def test_uses_its_own_checkout_outside_a_checkout(self):
        depot_tools = Path('/own/vendor/depot_tools')
        _, ensure, out = self._main(depot_tools=depot_tools)
        ensure.assert_called_once_with(runner._OWN_CHECKOUT)  # pylint: disable=protected-access
        self.assertEqual(out, f'{depot_tools / runner.VPYTHON3}\n')

    def test_prints_nothing_and_fails_when_deploy_fails(self):
        rc, _, out = self._main(checkout=Path('/w/src/brave'))
        self.assertEqual(rc, 1)
        self.assertEqual(out, '')


if __name__ == '__main__':
    unittest.main()
