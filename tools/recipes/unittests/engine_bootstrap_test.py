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


class MainForwardingTest(unittest.TestCase):
    """main() deploys the engine and launches it under vpython3, forwarding
    argv verbatim."""

    def test_forwards_argv_under_vpython(self):
        argv = ['toolchains/rust/package_rust', '--workspace', '/work']
        engine_path = Path('/work/recipes_engine_bootstrap/tools/recipes/'
                           'engine.py')
        spec_path = engine_path.parent / bootstrap.VPYTHON_SPEC
        with mock.patch.object(bootstrap, '_deploy_recipes',
                               return_value=engine_path) as deploy, \
             mock.patch.object(bootstrap.shutil, 'which',
                               return_value='/path/to/vpython3'), \
             mock.patch.object(bootstrap.subprocess, 'run') as run:
            run.return_value = types.SimpleNamespace(returncode=0)
            rc = bootstrap.main(argv)

        self.assertEqual(rc, 0)
        # The engine checkout is deployed under --workspace, and vpython3 was
        # on PATH, so the bare name is used (no depot_tools clone).
        deploy.assert_called_once_with(
            Path('/work') / bootstrap.RECIPES_ENGINE_DEST)
        forwarded = run.call_args[0][0]
        self.assertEqual(forwarded, [
            bootstrap.VPYTHON3, '-vpython-spec',
            str(spec_path), '-u',
            str(engine_path), *argv
        ])

    def test_missing_vpython_clones_depot_tools(self):
        """With no vpython3 on PATH, depot_tools is deployed under --workspace
        and its vpython3 (absolute path) is used to launch the engine."""
        engine_path = Path('/work/recipes_engine_bootstrap/tools/recipes/'
                           'engine.py')
        depot_tools = Path('/work/depot_tools_bootstrap')
        with mock.patch.dict(os.environ, clear=False), \
             mock.patch.object(bootstrap, '_deploy_recipes',
                               return_value=engine_path), \
             mock.patch.object(bootstrap, '_deploy_depot_tools',
                               return_value=depot_tools) as deploy_dt, \
             mock.patch.object(bootstrap.shutil, 'which', return_value=None), \
             mock.patch.object(bootstrap.subprocess, 'run') as run:
            run.return_value = types.SimpleNamespace(returncode=0)
            rc = bootstrap.main(
                ['toolchains/rust/package_rust', '--workspace', '/work'])
            # The cloned depot_tools is prepended to PATH (asserted inside the
            # patch.dict scope, which restores os.environ on exit) so the
            # engine and any recipe's depot_tools module reuse it.
            self.assertEqual(os.environ['PATH'].split(os.pathsep)[0],
                             str(depot_tools))

        self.assertEqual(rc, 0)
        # The clone destination is resolved relative to --workspace.
        deploy_dt.assert_called_once_with(
            Path('/work') / bootstrap.DEPOT_TOOLS_DEST)
        forwarded = run.call_args[0][0]
        self.assertEqual(forwarded[0], str(depot_tools / bootstrap.VPYTHON3))


class DeployRecipesTest(unittest.TestCase):
    """_deploy_recipes wipes the destination before cloning."""

    def test_nukes_existing_dest(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'bc'
            engine_file = dest / bootstrap.RECIPES_PATH / 'engine.py'
            engine_file.parent.mkdir(parents=True)
            engine_file.write_text('', encoding='utf-8', newline='')
            # The spec is validated too, so it must exist alongside the engine.
            (engine_file.parent / bootstrap.VPYTHON_SPEC).write_text(
                '', encoding='utf-8', newline='')
            # rmtree is mocked, so the pre-created engine file survives the
            # is_file() check; _run is mocked so no real git runs.
            with mock.patch.object(bootstrap, '_run') as run, \
                 mock.patch.object(bootstrap.shutil, 'rmtree') as rmtree:
                result = bootstrap._deploy_recipes(dest)
        rmtree.assert_called_once_with(dest.resolve())
        self.assertTrue(run.called)
        self.assertEqual(result, engine_file.resolve())


class DeployDepotToolsTest(unittest.TestCase):
    """_deploy_depot_tools reuses a good checkout and clones when absent."""

    def test_reuses_existing_vpython3(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'depot_tools'
            dest.mkdir()
            (dest / bootstrap.VPYTHON3).write_text('',
                                                   encoding='utf-8',
                                                   newline='')
            with mock.patch.object(bootstrap, '_run') as run:
                result = bootstrap._deploy_depot_tools(dest)
        run.assert_not_called()
        self.assertEqual(result, dest.resolve())

    def test_clones_when_absent(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'depot_tools'

            # _run is mocked, so simulate the clone producing a vpython3.
            def fake_clone(*_cmd, cwd=None):  # pylint: disable=unused-argument
                dest.mkdir(parents=True, exist_ok=True)
                (dest / bootstrap.VPYTHON3).write_text('',
                                                       encoding='utf-8',
                                                       newline='')

            with mock.patch.object(bootstrap, '_run',
                                   side_effect=fake_clone) as run:
                result = bootstrap._deploy_depot_tools(dest)
        run.assert_called_once()
        self.assertEqual(result, dest.resolve())


if __name__ == '__main__':
    unittest.main()
