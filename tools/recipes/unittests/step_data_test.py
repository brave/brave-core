#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for how StepData files placeholder results onto a step's result."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
from recipe_api import OutputPlaceholder
from step_data import StepData


def _placeholder(method='output', name=None, module='json'):
    """An output placeholder namespaced as the engine would namespace it."""
    placeholder = OutputPlaceholder(name=name)
    placeholder.namespaces = (module, method)
    return placeholder


def _finalized(*assignments):
    """A finalized StepData with (placeholder, result) pairs assigned."""
    data = StepData('a step', 0)
    for placeholder, result in assignments:
        data.assign_placeholder(placeholder, result)
    data.finalize()
    return data


class PlaceholderFilingTest(unittest.TestCase):

    def test_unnamed_placeholder_lands_under_its_namespaces(self):
        data = _finalized((_placeholder(), {'passed': 791}))
        self.assertEqual(data.json.output, {'passed': 791})

    def test_named_placeholders_land_under_the_plural(self):
        data = _finalized((_placeholder(name='a'), 1),
                          (_placeholder(name='b'), 2))
        self.assertEqual(data.json.outputs, {'a': 1, 'b': 2})
        # With more than one name and nothing unnamed, there is no default.
        with self.assertRaises(AttributeError):
            _ = data.json.output

    def test_a_lone_named_placeholder_is_also_the_default(self):
        data = _finalized((_placeholder(name='only'), 'value'))
        self.assertEqual(data.json.outputs, {'only': 'value'})
        self.assertEqual(data.json.output, 'value')

    def test_an_unnamed_placeholder_wins_the_default(self):
        data = _finalized((_placeholder(name='a'), 'named'),
                          (_placeholder(), 'unnamed'))
        self.assertEqual(data.json.output, 'unnamed')
        self.assertEqual(data.json.outputs, {'a': 'named'})

    def test_methods_and_modules_are_kept_apart(self):
        data = _finalized((_placeholder(), 'json output'),
                          (_placeholder(method='input'), 'json input'),
                          (_placeholder(module='raw_io'), 'raw_io output'))
        self.assertEqual(data.json.output, 'json output')
        self.assertEqual(data.json.input, 'json input')
        self.assertEqual(data.raw_io.output, 'raw_io output')

    def test_a_none_result_is_still_filed(self):
        # A placeholder whose file the step never wrote reports None, which is a
        # result like any other -- not an absent attribute.
        data = _finalized((_placeholder(), None))
        self.assertIsNone(data.json.output)

    def test_finalize_is_idempotent(self):
        data = _finalized((_placeholder(), 'value'))
        data.finalize()
        self.assertEqual(data.json.output, 'value')


class ErrorMessageTest(unittest.TestCase):

    def test_unknown_namespace_names_the_step(self):
        data = _finalized()
        with self.assertRaisesRegex(AttributeError, "'a step'.*'json'"):
            _ = data.json

    def test_unknown_attribute_names_the_namespace(self):
        data = _finalized((_placeholder(), 'value'))
        with self.assertRaisesRegex(AttributeError,
                                    r"'a step'\)\.json.*'nope'"):
            _ = data.json.nope

    def test_two_indistinguishable_placeholders_raise(self):
        data = StepData('a step', 0)
        data.assign_placeholder(_placeholder(), 'first')
        with self.assertRaisesRegex(ValueError, 'distinct names'):
            data.assign_placeholder(_placeholder(), 'second')


class FinalizedTest(unittest.TestCase):
    """A finalized result is read-only, so a recipe can't rewrite history."""

    def test_assigning_a_placeholder_after_finalize_raises(self):
        data = _finalized()
        with self.assertRaises(ValueError):
            data.assign_placeholder(_placeholder(), 'value')

    def test_assigning_an_attribute_after_finalize_raises(self):
        data = _finalized()
        with self.assertRaises(ValueError):
            data.retcode = 1

    def test_assigning_inside_a_namespace_after_finalize_raises(self):
        data = _finalized((_placeholder(), 'value'))
        with self.assertRaises(AttributeError):
            data.json.output = 'other'


class AlwaysPresentMembersTest(unittest.TestCase):

    def test_name_retcode_and_handles(self):
        data = StepData('a step', 3)
        self.assertEqual(data.name, 'a step')
        self.assertEqual(data.retcode, 3)
        # Unset until the step's stdout/stderr placeholders report back.
        self.assertIsNone(data.stdout)
        self.assertIsNone(data.stderr)


class PlaceholderLabelTest(unittest.TestCase):

    def test_label_reflects_the_name(self):
        self.assertEqual(_placeholder().label, 'json.output')
        self.assertEqual(_placeholder(name='cfg').label, 'json.output[cfg]')

    def test_repr_survives_an_unnamespaced_placeholder(self):
        self.assertEqual(repr(OutputPlaceholder()),
                         'OutputPlaceholder(<unnamespaced>)')
        self.assertEqual(repr(_placeholder(name='cfg')),
                         'OutputPlaceholder(json.output[cfg])')


if __name__ == '__main__':
    unittest.main()
