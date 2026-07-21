#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the recipe engine's DEPS resolution and property binding."""

# White-box tests: they exercise engine internals (`_Engine`,
# `_instantiate_module`, `_run_steps`, ...), so protected-access is
# intentional.
# pylint: disable=protected-access

import os
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import engine  # pylint: disable=wrong-import-position
from recipe_api import RecipeApi  # pylint: disable=wrong-import-position


class DepsResolutionTest(unittest.TestCase):
    """The engine wires a module's DEPS onto its injection site."""

    def test_wires_deps_onto_injection_site(self):
        cc = engine._Engine()._instantiate_module('chromium_checkout', [])
        self.assertTrue(hasattr(cc.m, 'depot_tools'))
        self.assertTrue(hasattr(cc.m, 'step'))
        # A module can reach itself via self.m.<own_name>.
        self.assertIs(cc.m.chromium_checkout, cc)

    def test_instances_are_cached_and_shared(self):
        eng = engine._Engine()
        cc = eng._instantiate_module('chromium_checkout', [])
        dt = eng._instantiate_module('depot_tools', [])
        # chromium_checkout and depot_tools share one `step` instance.
        self.assertIs(cc.m.step, dt.m.step)


class FindApiClassTest(unittest.TestCase):
    """_find_api_class requires exactly one RecipeApi subclass."""

    def test_zero_classes_raises(self):
        module = types.ModuleType('fake')
        with self.assertRaises(RuntimeError):
            engine._find_api_class(module, 'fake')

    def test_exactly_one_class(self):
        module = types.ModuleType('fake')

        class OnlyApi(RecipeApi):
            pass

        OnlyApi.__module__ = 'fake'
        module.OnlyApi = OnlyApi
        self.assertIs(engine._find_api_class(module, 'fake'), OnlyApi)

    def test_two_classes_raises(self):
        module = types.ModuleType('fake')

        class ApiA(RecipeApi):
            pass

        class ApiB(RecipeApi):
            pass

        ApiA.__module__ = ApiB.__module__ = 'fake'
        module.ApiA = ApiA
        module.ApiB = ApiB
        with self.assertRaises(RuntimeError):
            engine._find_api_class(module, 'fake')


class RunStepsBindingTest(unittest.TestCase):
    """_run_steps decodes input into typed PROPERTIES/ENV_PROPERTIES messages.

    Uses a recipe's real compiled proto messages, so it also exercises that the
    `PB` package builds and imports.
    """

    @classmethod
    def setUpClass(cls):
        engine._ensure_protos()
        # pylint: disable=import-outside-toplevel,import-error
        from PB.recipes.brave.toolchains.rust.package_rust import (
            EnvProperties, InputProperties)
        cls.InputProperties = InputProperties
        cls.EnvProperties = EnvProperties

    def _bind(self, properties, environ, props_def, env_def):
        """Return the positional args _run_steps would pass to RunSteps."""
        captured = []
        engine._run_steps(lambda *args: captured.extend(args), 'API',
                          properties, environ, props_def, env_def)
        return captured

    def test_no_defs_passes_only_api(self):
        self.assertEqual(self._bind({}, {}, None, None), ['API'])

    def test_properties_decoded_and_reserved_keys_stripped(self):
        args = self._bind(
            {
                'chromium_ref': 'main',
                'brave_subrevision': 3,
                '$hidden': 'ignored',
            }, {}, self.InputProperties, None)
        self.assertEqual(args[0], 'API')
        self.assertEqual(args[1].chromium_ref, 'main')
        self.assertEqual(args[1].brave_subrevision, 3)

    def test_env_properties_uppercased_and_unknown_ignored(self):
        args = self._bind({}, {
            'git_cache': '/c',
            'PATH': '/bin'
        }, None, self.EnvProperties)
        self.assertEqual(args[1].GIT_CACHE, '/c')

    def test_both_defs_pass_properties_then_env(self):
        args = self._bind({'chromium_ref': 'x'}, {'GIT_CACHE': '/c'},
                          self.InputProperties, self.EnvProperties)
        self.assertEqual(len(args), 3)
        self.assertEqual(args[1].chromium_ref, 'x')
        self.assertEqual(args[2].GIT_CACHE, '/c')

    def test_non_message_properties_def_raises(self):
        with self.assertRaises(TypeError):
            self._bind({}, {}, dict, None)


class RunRecipeTest(unittest.TestCase):
    """run_recipe maps the slash path to a module and requires RunSteps."""

    def test_slash_path_mapped_and_missing_run_steps_raises(self):
        fake_recipe = types.SimpleNamespace(DEPS=[])
        import_module = mock.Mock(return_value=fake_recipe)
        with mock.patch.object(engine.importlib, 'import_module',
                               import_module):
            with self.assertRaises(RuntimeError):
                engine.run_recipe('group/sub/my_recipe', {})
        import_module.assert_called_once_with('recipes.group.sub.my_recipe')


class WorkspaceTest(unittest.TestCase):
    """The engine seeds its workspace; the `path` module derives job paths."""

    def setUp(self):
        # _Engine chdirs into an explicit workspace; restore cwd afterwards so
        # it doesn't leak into other tests (some read Path.cwd()).
        self._prev_cwd = Path.cwd()
        self.addCleanup(os.chdir, self._prev_cwd)

    def test_workspace_seeded_into_path_module(self):
        with tempfile.TemporaryDirectory() as tmp:
            eng = engine._Engine(workspace=tmp)
            path = eng._instantiate_module('path', [])
            workspace = Path(tmp).resolve()
            self.assertEqual(path.workspace, workspace)
            self.assertEqual(path.chromium_src,
                             workspace / 'brave-browser/src')
            self.assertEqual(path.brave_core,
                             workspace / 'brave-browser/src/brave')
            self.assertEqual(path.out, workspace / 'out')
            self.assertEqual(Path.cwd(), workspace)

    def test_workspace_defaults_to_cwd(self):
        path = engine._Engine()._instantiate_module('path', [])
        self.assertEqual(path.workspace, Path.cwd())

    def test_brave_core_ref_seeded_with_override(self):
        module = engine._Engine(
            brave_core_ref='feature/x')._instantiate_module(
                'brave_core_checkout', [])
        self.assertEqual(getattr(module, '_brave_core_ref'), 'feature/x')

    def test_brave_core_ref_defaults_to_master(self):
        module = engine._Engine()._instantiate_module('brave_core_checkout',
                                                      [])
        self.assertEqual(getattr(module, '_brave_core_ref'), 'master')


if __name__ == '__main__':
    unittest.main()
