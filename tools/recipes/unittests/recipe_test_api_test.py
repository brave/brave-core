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
from recipe_test_api import (DisabledTestData, PlaceholderTestData,
                             RecipeTestApi, StepTestData, TestData,
                             placeholder_step_data)


def _step_test_data(retcode=None, **placeholders):
    """A StepTestData carrying *placeholders* (by name) and a retcode."""
    data = StepTestData()
    for name, value in placeholders.items():
        data.placeholder_data[('mod', 'output',
                               name)] = PlaceholderTestData(data=value,
                                                            name=name)
    data.retcode = retcode
    return data


class StepTestDataTest(unittest.TestCase):
    """StepTestData merges with explicitly-set fields on the right winning."""

    def test_placeholder_data_merges_per_key(self):
        merged = _step_test_data(a='first') + _step_test_data(b='second')
        self.assertEqual(
            {
                key[2]: datum.data
                for key, datum in merged.placeholder_data.items()
            }, {
                'a': 'first',
                'b': 'second'
            })

    def test_later_placeholder_datum_wins(self):
        merged = _step_test_data(a='first') + _step_test_data(a='second')
        self.assertEqual(
            merged.pop_placeholder('mod', 'output', 'a').data, 'second')

    def test_retcode_carries_and_unset_defaults_to_zero(self):
        self.assertEqual(StepTestData().retcode, 0)
        merged = _step_test_data(a='x') + _step_test_data(retcode=2)
        self.assertEqual(merged.retcode, 2)
        # The left side's placeholder datum survives.
        self.assertEqual(
            merged.pop_placeholder('mod', 'output', 'a').data, 'x')

    def test_conflicting_retcodes_raise(self):
        with self.assertRaises(ValueError):
            _ = _step_test_data(retcode=1) + _step_test_data(retcode=2)

    def test_override_discards_what_came_before(self):
        override = _step_test_data(b='second')
        override.override = True
        merged = _step_test_data(a='first', retcode=1) + override
        self.assertEqual(list(merged.placeholder_data),
                         [('mod', 'output', 'b')])
        self.assertEqual(merged.retcode, 0)

    def test_std_handles_default_to_an_empty_datum(self):
        data = StepTestData()
        # Unseeded handles still answer, so a placeholder can always ask.
        for handle in (data.stdin, data.stdout, data.stderr):
            self.assertIsNone(handle.data)
            self.assertTrue(handle.enabled)

    def test_unwrap_placeholder_requires_exactly_one(self):
        self.assertEqual(_step_test_data(a='x').unwrap_placeholder().data, 'x')
        with self.assertRaises(ValueError):
            _step_test_data(a='x', b='y').unwrap_placeholder()


class PlaceholderStepDataTest(unittest.TestCase):
    """The decorator wraps a plain triple into a keyed StepTestData."""

    # `functools.wraps` (in `placeholder_step_data`) leaves pylint inferring a
    # decorated method's return type from the *undecorated* function body, so
    # it sees the `(value, retcode, name)` tuple `output`/`invalid`/`delegating`
    # return rather than the `StepTestData` the decorator actually returns.
    # pylint: disable=no-member

    class FakeTestApi(RecipeTestApi):

        @placeholder_step_data
        def output(self, data, retcode=None, name=None):
            return f'data({data})', retcode, name

        @placeholder_step_data('output')
        def invalid(self, retcode=None, name=None):
            return 'garbage', retcode, name

        @placeholder_step_data
        def delegating(self, retcode=None, name=None):
            return self.output('hammer time', retcode=retcode, name=name)

    def setUp(self):
        self.api = self.FakeTestApi(module='mod')

    def test_keyed_by_module_method_and_name(self):
        data = self.api.output('hello', name='cool')
        self.assertEqual(list(data.placeholder_data),
                         [('mod', 'output', 'cool')])
        self.assertEqual(data.placeholder_data[('mod', 'output', 'cool')].data,
                         'data(hello)')

    def test_retcode_is_carried(self):
        self.assertEqual(self.api.output('hello', retcode=50).retcode, 50)

    def test_alternate_name_targets_another_method(self):
        # `invalid` seeds the placeholder `output` produces.
        self.assertEqual(list(self.api.invalid().placeholder_data),
                         [('mod', 'output', None)])

    def test_delegating_method_adopts_the_inner_datum(self):
        data = self.api.delegating(retcode=50, name='other')
        self.assertEqual(list(data.placeholder_data),
                         [('mod', 'delegating', 'other')])
        self.assertEqual(data.unwrap_placeholder().data, 'data(hammer time)')
        self.assertEqual(data.retcode, 50)

    def test_bad_decorator_arguments_raise(self):
        with self.assertRaises(ValueError):
            placeholder_step_data('')
        with self.assertRaises(ValueError):
            placeholder_step_data(123)


class DisabledTestDataTest(unittest.TestCase):
    """The production stand-in answers everything with itself, disabled."""

    def test_every_lookup_is_disabled(self):
        data = DisabledTestData()
        self.assertFalse(data.enabled)
        self.assertFalse(data.stdout.enabled)
        self.assertFalse(data.pop_placeholder('mod', 'output', None).enabled)


class StepTestDataLookupTest(unittest.TestCase):
    """A step's data merges its own default with what the case seeded."""

    def test_default_is_merged_under_the_seeded_data(self):
        case = TestData('case') + RecipeTestApi.step_data(
            's', _step_test_data(a='seeded'))
        data = case.get_step_test_data('s',
                                       lambda: _step_test_data(b='default'))
        self.assertEqual(
            data.pop_placeholder('mod', 'output', 'a').data, 'seeded')
        self.assertEqual(
            data.pop_placeholder('mod', 'output', 'b').data, 'default')

    def test_override_discards_the_default(self):
        case = TestData('case') + RecipeTestApi.override_step_data(
            's', _step_test_data(a='seeded'))
        data = case.get_step_test_data('s',
                                       lambda: _step_test_data(b='default'))
        self.assertIsNone(data.pop_placeholder('mod', 'output', 'b').data)

    def test_a_repeated_step_name_sees_the_data_each_time(self):
        # Step names are not deduplicated by this engine, so seeded data has to
        # survive being handed out more than once.
        case = TestData('case') + RecipeTestApi.step_data(
            's', _step_test_data(a='seeded'))
        for _ in range(2):
            data = case.get_step_test_data('s', StepTestData)
            self.assertEqual(
                data.pop_placeholder('mod', 'output', 'a').data, 'seeded')

    def test_conflicting_retcodes_name_the_step(self):
        case = TestData('case') + RecipeTestApi.step_data('s', retcode=1)
        with self.assertRaisesRegex(ValueError, "in step 's'"):
            case.get_step_test_data('s', lambda: _step_test_data(retcode=2))


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
                  RecipeTestApi.step_data(
                      's', stdout=_step_test_data(out='written')))
        self.assertEqual(merged.step_data['s'].retcode, 1)
        self.assertEqual(merged.step_data['s'].stdout.data, 'written')

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
