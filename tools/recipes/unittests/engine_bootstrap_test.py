#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the self-contained engine bootstrap."""

# White-box tests: they exercise bootstrap internals (`_deploy_recipes`,
# `_brave_core_dest`, `_run`), so protected-access is intentional.
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


class BraveCoreDestTest(unittest.TestCase):
    """The checkout destination is read from --properties (or defaulted)."""

    def test_default(self):
        self.assertEqual(bootstrap._brave_core_dest('{}'), 'brave_core')

    def test_explicit(self):
        self.assertEqual(
            bootstrap._brave_core_dest('{"brave_core_dest": "scratch/bc"}'),
            'scratch/bc')

    def test_unparseable_json_falls_back(self):
        self.assertEqual(bootstrap._brave_core_dest('not json'), 'brave_core')

    def test_non_object_json_falls_back(self):
        self.assertEqual(bootstrap._brave_core_dest('[1, 2]'), 'brave_core')


class MainForwardingTest(unittest.TestCase):
    """main() deploys the engine and forwards argv to it verbatim."""

    def test_forwards_argv_verbatim(self):
        argv = [
            'toolchains/rust/package_rust', '--properties',
            '{"brave_core_dest": "bc"}'
        ]
        engine_path = Path('bc/tools/recipes/engine.py')
        with mock.patch.object(bootstrap, '_deploy_recipes',
                               return_value=engine_path) as deploy, \
             mock.patch.object(bootstrap.subprocess, 'run') as run:
            run.return_value = types.SimpleNamespace(returncode=0)
            rc = bootstrap.main(argv)

        self.assertEqual(rc, 0)
        deploy.assert_called_once_with('bc')
        forwarded = run.call_args[0][0]
        self.assertEqual(forwarded[0], sys.executable)
        self.assertEqual(forwarded[1:], [str(engine_path), *argv])


class DeployRecipesTest(unittest.TestCase):
    """_deploy_recipes wipes the destination before cloning."""

    def test_nukes_existing_dest(self):
        with tempfile.TemporaryDirectory() as work:
            dest = Path(work) / 'bc'
            engine_file = dest / bootstrap.RECIPES_PATH / 'engine.py'
            engine_file.parent.mkdir(parents=True)
            engine_file.write_text('', encoding='utf-8', newline='')
            # rmtree is mocked, so the pre-created engine file survives the
            # is_file() check; _run is mocked so no real git runs.
            with mock.patch.object(bootstrap, '_run') as run, \
                 mock.patch.object(bootstrap.shutil, 'rmtree') as rmtree:
                result = bootstrap._deploy_recipes(dest)
        rmtree.assert_called_once_with(dest.resolve())
        self.assertTrue(run.called)
        self.assertEqual(result, engine_file.resolve())


if __name__ == '__main__':
    unittest.main()
