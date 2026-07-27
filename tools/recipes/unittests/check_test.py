#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the AST-introspecting check machinery in check.py."""

import os
import re
import sys
import unittest
from collections import OrderedDict

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
from check import (Checker, CheckFrame, PostProcessError, VerifySubset,
                   render_re)
from recipe_test_api import PostprocessHookContext


def _noop(check, steps):
    del check, steps


# A hook context standing in for an `api.post_process(...)` registration site.
HOOK = PostprocessHookContext(_noop, ('arg', ), {'k': 'v'}, '<caller>', 7)


class CheckerTest(unittest.TestCase):
    """The Checker records failures with AST-rendered source and values."""

    def _sanitize(self, frame):
        # File/line vary by machine; only the code + varmap are asserted.
        return frame._replace(line=0, fname='')

    def test_no_calls(self):
        checker = Checker(HOOK)

        def body(check):
            del check

        body(checker)
        self.assertEqual(checker.failed_checks, [])

    def test_passing_call_records_nothing(self):
        checker = Checker(HOOK)

        def body(check):
            check(True)
            check('hint', 1 < 2)

        body(checker)
        self.assertEqual(checker.failed_checks, [])

    def test_failing_call_renders_source(self):
        checker = Checker(HOOK)

        def body(check):
            check(1 == 2)

        body(checker)
        self.assertEqual(len(checker.failed_checks), 1)
        frame = checker.failed_checks[0].frames[-1]
        self.assertEqual(
            self._sanitize(frame),
            CheckFrame(fname='',
                       line=0,
                       function='body',
                       code='check(1 == 2)',
                       varmap={}))

    def test_hint_is_recorded_as_name(self):
        checker = Checker(HOOK)

        def body(check):
            check('the hint', False)

        body(checker)
        self.assertEqual(checker.failed_checks[0].name, 'the hint')

    def test_membership_renders_dict_keys(self):
        checker = Checker(HOOK)

        def body(check):
            steps = {'b': 1, 'a': 2}
            check('x' in steps)

        body(checker)
        varmap = checker.failed_checks[0].frames[-1].varmap
        # The dict itself is elided in favour of its (sorted) keys.
        self.assertEqual(varmap, {'steps.keys()': "['a', 'b']"})

    def test_subscript_resolves_value(self):
        checker = Checker(HOOK)

        def body(check):
            data = {'k': 'v'}
            check(data['k'] == 'other')

        body(checker)
        varmap = checker.failed_checks[0].frames[-1].varmap
        self.assertEqual(varmap["data['k']"], "'v'")

    def test_local_variable_is_rendered(self):
        checker = Checker(HOOK)

        def body(check):
            name = 'missing'
            check(name == 'present')

        body(checker)
        varmap = checker.failed_checks[0].frames[-1].varmap
        self.assertEqual(varmap['name'], "'missing'")

    def test_format_block(self):
        checker = Checker(HOOK)

        def body(check):
            check('must exist', False)

        body(checker)
        lines = checker.failed_checks[0].format()
        self.assertEqual(lines[0], "CHECK 'must exist' (FAIL):")
        # The footer names the registration site and the call repr.
        self.assertIn('added <caller>:7', lines[-2])
        self.assertEqual(lines[-1].strip(), "_noop('arg', k='v')")


class VerifySubsetTest(unittest.TestCase):
    """VerifySubset accepts subsets and describes any violation."""

    def test_identical_and_subset(self):
        self.assertIsNone(VerifySubset({'a': 1}, {'a': 1}))
        self.assertIsNone(VerifySubset({'a': 1}, {'a': 1, 'b': 2}))

    def test_added_key(self):
        self.assertEqual(VerifySubset({
            'a': 1,
            'c': 3
        }, {'a': 1}), ": added key 'c'")

    def test_value_mismatch(self):
        self.assertEqual(VerifySubset({'a': 'x'}, {'a': 'y'}),
                         "['a']: 'x' != 'y'")

    def test_type_mismatch(self):
        self.assertIn('type mismatch', VerifySubset({'a': 1}, {'a': [1]}))

    def test_list_too_long(self):
        self.assertIn('too long', VerifySubset([1, 2, 3], [1, 2]))

    def test_ordered_out_of_order(self):
        a = OrderedDict([('b', 1), ('a', 2)])
        b = OrderedDict([('a', 2), ('b', 1)])
        self.assertIn('out of order', VerifySubset(a, b))

    def test_empty_dict_is_subset_of_ordereddict(self):
        self.assertIsNone(VerifySubset({}, OrderedDict([('a', 1)])))


class RenderReTest(unittest.TestCase):

    def test_plain_and_flagged(self):
        # Python's `re` carries the UNICODE flag by default on str patterns.
        self.assertEqual(render_re(re.compile('foo')),
                         "re.compile('foo', UNICODE)")
        self.assertIn('IGNORECASE', render_re(re.compile('foo', re.I)))


class PostProcessErrorTest(unittest.TestCase):

    def test_is_value_error(self):
        self.assertTrue(issubclass(PostProcessError, ValueError))


if __name__ == '__main__':
    unittest.main()
