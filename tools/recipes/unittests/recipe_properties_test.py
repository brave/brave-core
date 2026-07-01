#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the `from_environ` recipe property mechanism."""

# White-box tests: they exercise internals (`_coerce`), so protected-access is
# intentional.
# pylint: disable=protected-access

from __future__ import annotations

import os
import sys
import unittest
from dataclasses import dataclass
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import engine
import recipe_properties
from recipe_properties import Property, apply_environ


@dataclass(frozen=True)
class _Props:
    chromium_ref: str = Property(from_environ='CHROMIUM_REF')
    subrevision: int = Property(default=0, from_environ='SUBREVISION')
    flag: bool = Property(default=False, from_environ='FLAG')
    git_cache: str | None = Property(default=None)


class ApplyEnvironTest(unittest.TestCase):
    """apply_environ backfills fields from the environment."""

    def test_explicit_value_wins_over_environ(self):
        merged = apply_environ(_Props, {'chromium_ref': 'explicit'},
                               {'CHROMIUM_REF': 'from_env'})
        self.assertEqual(merged['chromium_ref'], 'explicit')

    def test_reads_from_environ_when_absent(self):
        merged = apply_environ(_Props, {}, {'CHROMIUM_REF': 'main'})
        self.assertEqual(merged['chromium_ref'], 'main')

    def test_missing_env_leaves_field_unset(self):
        merged = apply_environ(_Props, {}, {})
        self.assertNotIn('chromium_ref', merged)

    def test_coerces_int_and_bool(self):
        merged = apply_environ(_Props, {}, {
            'SUBREVISION': '7',
            'FLAG': 'true'
        })
        self.assertEqual(merged['subrevision'], 7)
        self.assertIs(merged['flag'], True)

    def test_field_without_from_environ_is_ignored(self):
        merged = apply_environ(_Props, {}, {'GIT_CACHE': '/tmp/cache'})
        self.assertNotIn('git_cache', merged)

    def test_non_dataclass_returns_values_unchanged(self):
        merged = apply_environ(None, {'a': 1}, {'CHROMIUM_REF': 'main'})
        self.assertEqual(merged, {'a': 1})


class BuildPropertiesEnvironTest(unittest.TestCase):
    """_build_properties honours from_environ end to end."""

    def test_env_backfills_required_property(self):
        with mock.patch.dict(os.environ, {'CHROMIUM_REF': 'main'}, clear=True):
            obj = engine._build_properties(_Props, {})
        self.assertEqual(obj.chromium_ref, 'main')
        self.assertEqual(obj.subrevision, 0)

    def test_explicit_overrides_env(self):
        with mock.patch.dict(os.environ, {'CHROMIUM_REF': 'from_env'},
                             clear=True):
            obj = engine._build_properties(_Props,
                                           {'chromium_ref': 'explicit'})
        self.assertEqual(obj.chromium_ref, 'explicit')

    def test_missing_required_property_raises(self):
        with mock.patch.dict(os.environ, {}, clear=True):
            with self.assertRaises(TypeError):
                engine._build_properties(_Props, {})


class CoerceTest(unittest.TestCase):
    """_coerce converts environment strings to simple scalar types."""

    def test_int(self):
        self.assertEqual(recipe_properties._coerce('42', 'int'), 42)

    def test_bool_truthy(self):
        for value in ('1', 'true', 'YES', 'On'):
            self.assertIs(recipe_properties._coerce(value, 'bool'), True)

    def test_bool_falsey(self):
        for value in ('0', 'false', '', 'no'):
            self.assertIs(recipe_properties._coerce(value, 'bool'), False)

    def test_str_passthrough(self):
        self.assertEqual(recipe_properties._coerce('main', 'str'), 'main')
        self.assertEqual(recipe_properties._coerce('x', 'str | None'), 'x')


if __name__ == '__main__':
    unittest.main()
