#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the test runner's TEST_API discovery/DI and recipe discovery."""

# White-box tests exercising runner internals; protected access is intentional.
# pylint: disable=protected-access

import os
import sys
import types
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import recipe_test_runner as runner
from recipe_test_api import RecipeTestApi


class FindTestApiClassTest(unittest.TestCase):

    def test_defaults_to_base_when_absent(self):
        module = types.ModuleType('fake')
        self.assertIs(runner._find_test_api_class(module, 'fake'),
                      RecipeTestApi)

    def test_finds_the_single_subclass(self):
        module = types.ModuleType('fake')

        class MyTestApi(RecipeTestApi):
            pass

        MyTestApi.__module__ = 'fake'
        module.MyTestApi = MyTestApi
        self.assertIs(runner._find_test_api_class(module, 'fake'), MyTestApi)


class TestApiInjectionTest(unittest.TestCase):

    def test_deps_are_wired_onto_m(self):
        cache = {}
        cc = runner._instantiate_test_module('chromium_checkout', [], cache)
        # chromium_checkout's DEPS test apis are reachable via .m.
        self.assertTrue(hasattr(cc.m, 'env'))
        self.assertTrue(hasattr(cc.m, 'path'))
        # Cached instances are shared.
        self.assertIs(runner._instantiate_test_module('path', [], cache),
                      cc.m.path)

    def test_root_api_exposes_dep_helpers(self):
        root = runner._build_root_test_api(['platform', 'step'])
        # api.platform.name(...) and api.step.data(...) resolve to the module
        # test apis, mirroring the recipes_py example.
        self.assertEqual(
            root.platform.name('mac').mod_data['platform']['name'], 'mac')
        self.assertIn('s', root.step.data('s', retcode=2).step_data)


class DiscoveryTest(unittest.TestCase):

    def test_iter_recipe_ids_includes_recipes_and_examples(self):
        ids = runner._iter_recipe_ids()
        self.assertIn('toolchains/rust/package_rust', ids)
        self.assertIn('platform/examples/full', ids)

    def test_testable_recipes_have_gentests(self):
        recipes = dict(runner._testable_recipes())
        self.assertIn('tools/node/package_node', recipes)
        for recipe in recipes.values():
            self.assertTrue(hasattr(recipe, 'GenTests'))


if __name__ == '__main__':
    unittest.main()
