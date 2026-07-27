#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for TestData/StepTestData merging and the RecipeTestApi builders."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
from recipe_test_api import RecipeTestApi, StepTestData, TestData


class StepTestDataTest(unittest.TestCase):
    """StepTestData merges with explicitly-set fields on the right winning."""

    def test_retcode_and_output_override(self):
        base = StepTestData(retcode=0, stdout='a')
        override = StepTestData(retcode=2)
        merged = base + override
        self.assertEqual(merged.retcode, 2)
        self.assertEqual(merged.stdout, 'a')  # not overridden -> kept

    def test_unset_retcode_defaults_to_zero(self):
        self.assertEqual(StepTestData().retcode, 0)


class TestDataMergeTest(unittest.TestCase):
    """TestData.__add__ is an associative merge of the fragment aspects."""

    def test_properties_and_status_merge(self):
        merged = (RecipeTestApi.properties(a=1) +
                  RecipeTestApi.properties(b=2))
        self.assertEqual(merged.properties, {'a': 1, 'b': 2})

    def test_mod_data_lists_concatenate(self):
        api = _module_api('path')
        merged = api.files('one') + api.files('two')
        self.assertEqual(merged.mod_data['path']['files'], ['one', 'two'])

    def test_mod_data_dicts_merge(self):
        api = _module_api('env')
        merged = api.set('A', '1') + api.set('B', '2')
        self.assertEqual(merged.mod_data['env']['vars'], {'A': '1', 'B': '2'})

    def test_step_data_merges_per_step(self):
        merged = (RecipeTestApi.step_data('s', retcode=1) +
                  RecipeTestApi.step_data('s', stdout='out'))
        self.assertEqual(merged.step_data['s'].retcode, 1)
        self.assertEqual(merged.step_data['s'].stdout, 'out')

    def test_hooks_concatenate(self):
        merged = (RecipeTestApi.post_process(lambda c, s: None) +
                  RecipeTestApi.post_process(lambda c, s: None))
        self.assertEqual(len(merged.post_process_hooks), 2)


class RecipeTestApiTest(unittest.TestCase):
    """The root api folds fragments and records call sites for hooks."""

    def test_test_folds_fragments_and_status(self):
        td = RecipeTestApi.test('case',
                                RecipeTestApi.properties(x=1),
                                status='FAILURE')
        self.assertEqual(td.name, 'case')
        self.assertEqual(td.properties, {'x': 1})
        self.assertEqual(td.expected_status, 'FAILURE')

    def test_post_process_records_context(self):
        td = RecipeTestApi.post_process(lambda c, s: None)
        hook = td.post_process_hooks[0]
        self.assertIn('recipe_test_api_test.py', hook.filename)
        self.assertGreater(hook.lineno, 0)

    def test_root_is_its_own_injection_site(self):
        root = RecipeTestApi(module=None)
        self.assertIs(root.m, root)

    def test_mod_data_requires_a_module(self):
        with self.assertRaises(AssertionError):
            RecipeTestApi(module=None)._mod_data(x=1)  # pylint: disable=protected-access


def _module_api(name):
    """A module test api instance (as the runner would build it)."""
    from importlib import import_module
    module = import_module(f'recipe_modules.{name}.test_api')
    for value in vars(module).values():
        if (isinstance(value, type) and issubclass(value, RecipeTestApi)
                and value is not RecipeTestApi
                and value.__module__ == module.__name__):
            return value(module=name)
    raise AssertionError(f'no test api in {name}')


if __name__ == '__main__':
    unittest.main()
