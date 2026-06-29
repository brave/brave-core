#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the recipe engine's DEPS resolution and property binding."""

import os
import sys
import types
import unittest
from dataclasses import dataclass
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


class BuildPropertiesTest(unittest.TestCase):
    """_build_properties maps a dict onto PROPERTIES (or a namespace)."""

    def test_no_properties_def_returns_namespace(self):
        obj = engine._build_properties(None, {'a': 1, 'b': 'x'})
        self.assertEqual((obj.a, obj.b), (1, 'x'))

    def test_dataclass_def_applies_defaults(self):

        @dataclass
        class Props:
            a: int
            b: str = 'default'

        obj = engine._build_properties(Props, {'a': 5})
        self.assertEqual((obj.a, obj.b), (5, 'default'))


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


if __name__ == '__main__':
    unittest.main()
