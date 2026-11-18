#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for validate.py's own logic: the checks across builders and against
the machine's secrets, and `cmd_validate()`'s success/failure paths. Each
builder's own output is checked as generated_output.py reads it, tested in
generated_output_test.py. `bots.py`'s CLI wiring for the `validate`
subcommand is exercised in bots_test.py instead."""

import argparse
import contextlib
import io
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import generated_output
import testing_support
import validate
from testing_support import write_builder


def _gn_args_with_secret(value):
    return {
        'gn_args': {**testing_support.GN_ARGS['gn_args'], 'unrelated': value},
        'secrets': {'fake_secret_key': 'FAKE_SECRET_ENV_VAR'},
    }


class FindProblemsTest(unittest.TestCase):
    def setUp(self):
        self.output_dir = testing_support.use_output_dir(self)

    def test_no_generated_builders_says_run_snapshot_first(self):
        problems = validate.find_problems()
        self.assertEqual(len(problems), 1)
        self.assertIn('bots.py snapshot', problems[0])

    def test_valid_builder_has_no_problems(self):
        write_builder(self.output_dir, 'b')
        self.assertEqual(validate.find_problems(), [])

    def test_a_builders_own_problems_are_reported(self):
        write_builder(self.output_dir, 'b', omit=('targets.json',))
        problems = validate.find_problems()
        self.assertEqual(len(problems), 1)
        self.assertIn('targets.json: missing', problems[0])

    def test_builders_sharing_gn_args_but_not_targets_are_fine(self):
        # The two real ASan builders resolve to identical gn_args by design
        # and differ only in targets.json.
        write_builder(
            self.output_dir,
            'a',
            targets={'compile': ['brave:all'], 'tests': ['brave_unit_tests']},
        )
        write_builder(
            self.output_dir,
            'b',
            targets={'compile': ['brave:all'], 'tests': ['chromium_tests']},
        )
        self.assertEqual(validate.find_problems(), [])

    def test_duplicate_builders_are_reported(self):
        # Their `properties.json` differ only in naming each its builder.
        write_builder(self.output_dir, 'b1')
        write_builder(self.output_dir, 'b2')
        problems = validate.find_problems()
        self.assertEqual(len(problems), 1)
        self.assertIn("'b1' and 'b2' are exact duplicates", problems[0])

    def test_leaked_secret_value_is_reported(self):
        testing_support.use_dotenv(self, 'fake_secret_key=super-secret\n')
        write_builder(
            self.output_dir, 'b', gn_args=_gn_args_with_secret('super-secret')
        )
        problems = validate.find_problems()
        self.assertEqual(len(problems), 1)
        self.assertIn("gn_args['unrelated']", problems[0])
        self.assertIn("'fake_secret_key'", problems[0])
        self.assertNotIn('super-secret', problems[0])

    def test_unset_dummy_placeholder_is_not_flagged_as_leaked(self):
        # An unset secret and an unrelated gn_arg can both legitimately be
        # "dummy", and that coincidence must not read as one leaking into the
        # other.
        testing_support.use_dotenv(self, 'fake_secret_key=dummy\n')
        write_builder(
            self.output_dir, 'b', gn_args=_gn_args_with_secret('dummy')
        )
        self.assertEqual(validate.find_problems(), [])

    def test_secret_declared_but_not_in_dotenv_is_fine(self):
        testing_support.use_dotenv(self, '')
        write_builder(
            self.output_dir, 'b', gn_args=_gn_args_with_secret('anything')
        )
        self.assertEqual(validate.find_problems(), [])


class CmdValidateTest(unittest.TestCase):
    def setUp(self):
        self.output_dir = testing_support.use_output_dir(self)

    def _validate(self, quiet):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            ret = validate.cmd_validate(argparse.Namespace(quiet=quiet))
        self.assertEqual(ret, 0)
        return buf.getvalue()

    def test_prints_ok_message(self):
        write_builder(self.output_dir, 'b')
        output = self._validate(quiet=False)
        self.assertIn('looks ok', output)
        self.assertIn('1 builder', output)

    def test_quiet_suppresses_success_message(self):
        write_builder(self.output_dir, 'b')
        self.assertEqual(self._validate(quiet=True), '')

    def test_raises_bots_error_with_every_problem(self):
        write_builder(self.output_dir, 'b1')
        write_builder(self.output_dir, 'b2')
        write_builder(self.output_dir, 'c', omit=('sync.json',))
        with self.assertRaises(generated_output.BotsError) as ctx:
            self._validate(quiet=False)
        self.assertIn('duplicates', str(ctx.exception))
        self.assertIn('sync.json: missing', str(ctx.exception))


if __name__ == '__main__':
    unittest.main()
