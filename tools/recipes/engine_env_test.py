#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `engine_env.merge_envs` (the env composition). Covers prefix/suffix
joining, override folding, `%(VAR)s` substitution, and variable removal."""

from __future__ import annotations

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import engine_env as m


class MergeEnvsTest(unittest.TestCase):

    def test_no_modifications_returns_copy(self):
        original = {'A': '1'}
        result = m.merge_envs(original, {}, {}, {}, ':')
        self.assertEqual(result, {'A': '1'})
        self.assertIsNot(result, original)

    def test_prefix_prepends_to_existing_value(self):
        result = m.merge_envs({'PATH': '/bin'}, {}, {'PATH': ['/opt']}, {},
                              ':')
        self.assertEqual(result['PATH'], '/opt:/bin')

    def test_prefix_when_variable_absent(self):
        # No existing value: only the prefix, no dangling separator.
        result = m.merge_envs({}, {}, {'PATH': ['/opt']}, {}, ':')
        self.assertEqual(result['PATH'], '/opt')

    def test_suffix_appends(self):
        result = m.merge_envs({'PATH': '/bin'}, {}, {}, {'PATH': ['/z']}, ':')
        self.assertEqual(result['PATH'], '/bin:/z')

    def test_prefix_and_suffix_bracket_the_value(self):
        result = m.merge_envs({'PATH': '/bin'}, {}, {'PATH': ['/a']},
                              {'PATH': ['/z']}, ':')
        self.assertEqual(result['PATH'], '/a:/bin:/z')

    def test_override_folds_into_prefix(self):
        # When a key has both an override and a prefix, the override value is
        # folded in as the last prefix component (replacing the original value).
        result = m.merge_envs({'PATH': '/bin'}, {'PATH': '/custom'},
                              {'PATH': ['/a']}, {}, ':')
        self.assertEqual(result['PATH'], '/a:/custom')

    def test_override_substitution_amends_existing(self):
        result = m.merge_envs({'PATH': '/bin'}, {'PATH': '%(PATH)s:/x'}, {},
                              {}, ':')
        self.assertEqual(result['PATH'], '/bin:/x')

    def test_unknown_substitution_is_empty(self):
        result = m.merge_envs({}, {'X': '%(MISSING)s-y'}, {}, {}, ':')
        self.assertEqual(result['X'], '-y')

    def test_override_none_removes_variable(self):
        result = m.merge_envs({'A': '1', 'B': '2'}, {'A': None}, {}, {}, ':')
        self.assertNotIn('A', result)
        self.assertEqual(result['B'], '2')

    def test_pathsep_is_honoured(self):
        result = m.merge_envs({'PATH': 'C:\\bin'}, {}, {'PATH': ['C:\\opt']},
                              {}, ';')
        self.assertEqual(result['PATH'], 'C:\\opt;C:\\bin')


if __name__ == '__main__':
    unittest.main()
