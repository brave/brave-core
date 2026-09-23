#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the test runner's recipe discovery.

TEST_API discovery and injection live in `engine` (every run builds them, not
just simulated ones), and are tested in `engine_test.py`.
"""

# White-box tests exercising runner internals; protected access is intentional.
# pylint: disable=protected-access

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import recipe_test_runner as runner  # pylint: disable=wrong-import-position


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
