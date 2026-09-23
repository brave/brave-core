#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit gate: every recipe's expectations must be current.

Runs the full recipe simulation suite in compare mode (equivalent to
`engine.py test run`) and fails if any expectation is stale or missing, so a
recipe/module change that alters the step stream can't land without retraining
(`engine.py test train`). The finer-grained machinery tests live alongside this
file (`*_test.py`); this one asserts the goldens themselves.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import recipe_test_runner  # pylint: disable=wrong-import-position


class RecipeExpectationsTest(unittest.TestCase):

    def test_all_expectations_are_current(self):
        exit_code = recipe_test_runner.run_tests(train=False)
        self.assertEqual(
            exit_code, 0,
            'recipe expectations are stale or a check failed; run '
            '`python3 tools/recipes/engine.py test train` and review the diff')


if __name__ == '__main__':
    unittest.main()
