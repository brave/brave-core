#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the RecipeApi base class and the placeholder protocol."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
from recipe_api import (InputPlaceholder, OutputPlaceholder, Placeholder,
                        RecipeApi, returns_placeholder)


class FakeApi(RecipeApi):
    """A module api handing out placeholders, as a real one would."""

    @returns_placeholder
    def output(self, name=None):
        return OutputPlaceholder(name=name)

    @returns_placeholder('output')
    def leaky_output(self):
        """Produces the same placeholder `output` does, under its name."""
        return OutputPlaceholder()

    @returns_placeholder
    def not_a_placeholder(self):
        return 'nope'


def _api(module_name='fake'):
    api = FakeApi()
    # The engine seeds the module's registered name; placeholders are
    # namespaced with it.
    setattr(api, '_module_name', module_name)
    return api


class ReturnsPlaceholderTest(unittest.TestCase):

    def test_namespaces_are_the_module_and_method(self):
        self.assertEqual(_api().output().namespaces, ('fake', 'output'))

    def test_alternate_name_substitutes_the_method(self):
        self.assertEqual(_api().leaky_output().namespaces, ('fake', 'output'))

    def test_the_placeholder_name_is_carried_through(self):
        self.assertEqual(_api().output(name='cfg').name, 'cfg')

    def test_docstring_and_name_are_preserved(self):
        self.assertEqual(FakeApi.leaky_output.__name__, 'leaky_output')
        self.assertIn('same placeholder', FakeApi.leaky_output.__doc__)

    def test_a_method_that_returns_something_else_is_a_bug(self):
        with self.assertRaises(AssertionError):
            _api().not_a_placeholder()

    def test_bad_decorator_arguments_raise(self):
        with self.assertRaises(ValueError):
            returns_placeholder('')
        with self.assertRaises(ValueError):
            returns_placeholder(123)


class PlaceholderTest(unittest.TestCase):

    def test_a_non_string_name_is_rejected(self):
        # The name is what tells several placeholders apart on one step.
        with self.assertRaises(ValueError):
            Placeholder(name=123)

    def test_the_base_class_renders_nothing(self):
        # Subclasses supply both; the base exists to be implemented.
        placeholder = Placeholder()
        with self.assertRaises(NotImplementedError):
            _ = placeholder.backing_file
        with self.assertRaises(NotImplementedError):
            placeholder.render(None)

    def test_placeholders_stand_for_a_file_by_default(self):
        # Only the ones standing for something else (a whole directory) opt out,
        # and those are rejected on a step's std handles.
        self.assertTrue(Placeholder().is_file_backed)

    def test_input_and_output_hooks_default_to_doing_nothing(self):
        # A placeholder with no file to clean up, or no data to report, doesn't
        # have to say so.
        self.assertIsNone(InputPlaceholder().cleanup(True))
        self.assertIsNone(OutputPlaceholder().result(None))


class ModuleInjectionTest(unittest.TestCase):

    def test_an_undeclared_dependency_says_so(self):
        with self.assertRaisesRegex(AttributeError, 'DEPS'):
            _ = RecipeApi().m.nope

    def test_test_api_is_unset_until_the_engine_seeds_it(self):
        self.assertIsNone(RecipeApi().test_api)


if __name__ == '__main__':
    unittest.main()
