#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for generated_output.py: listing the generated builders and reading
their output back. Rendering it is tested through `bots.py snapshot`, in
snapshot_test.py."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import generated_output
import testing_support
from testing_support import write_builder


class ListGeneratedBuildersTest(unittest.TestCase):
    def test_no_output_dir_means_no_builders(self):
        output_dir = testing_support.use_output_dir(self)
        output_dir.rmdir()
        self.assertEqual(generated_output.list_generated_builders(), [])

    def test_lists_builders_sorted_by_name(self):
        output_dir = testing_support.use_output_dir(self)
        write_builder(output_dir, 'z-builder')
        write_builder(output_dir, 'a-builder')
        self.assertEqual(
            generated_output.list_generated_builders(),
            ['a-builder', 'z-builder'],
        )


class GeneratedBuilderTest(unittest.TestCase):
    def setUp(self):
        self.output_dir = testing_support.use_output_dir(self)

    def _read(self):
        return generated_output.GeneratedBuilder('b').read()

    def _problems(self):
        return generated_output.GeneratedBuilder('b').problems()

    def test_reads_every_file(self):
        write_builder(self.output_dir, 'b')
        output = self._read()
        self.assertEqual(output.gn_args, testing_support.GN_ARGS)
        self.assertEqual(output.builder_spec, testing_support.BUILDER_SPEC)
        self.assertEqual(output.sync, testing_support.SYNC)
        self.assertEqual(output.targets, testing_support.TARGETS)
        self.assertEqual(self._problems(), [])

    def test_unknown_builder_lists_available_ones(self):
        write_builder(self.output_dir, 'a-builder')
        with self.assertRaisesRegex(
            generated_output.BotsError,
            "'b' not found .* available builders: a-builder.",
        ):
            self._read()
        self.assertEqual(len(self._problems()), 1)

    def test_problems_are_reported_per_file(self):
        write_builder(
            self.output_dir,
            'b',
            gn_args={'gn_args': {}},
            targets={'compile': 'brave:all', 'tests': []},
        )
        problems = self._problems()
        self.assertEqual(len(problems), 2)
        self.assertIn(
            "gn-args.json: \"gn_args\" is missing 'target_os'; ", problems[0]
        )
        self.assertIn("targets.json: 'compile' is not a list", problems[1])
        with self.assertRaisesRegex(generated_output.BotsError, 'gn-args'):
            self._read()

    def test_missing_file(self):
        write_builder(self.output_dir, 'b', omit=('targets.json',))
        self.assertRegex(self._problems()[0], 'targets.json: missing$')

    def test_invalid_json(self):
        write_builder(self.output_dir, 'b')
        (self.output_dir / 'b' / 'gn-args.json').write_text('not json')
        self.assertIn('gn-args.json: invalid JSON', self._problems()[0])

    def test_not_a_json_object(self):
        write_builder(self.output_dir, 'b', sync=[])
        self.assertIn('expected a JSON object, got list', self._problems()[0])

    def test_gn_args_not_an_object(self):
        for gn_args in ({}, {'gn_args': 'nope'}):
            with self.subTest(gn_args=gn_args):
                write_builder(self.output_dir, 'b', gn_args=gn_args)
                self.assertIn(
                    '"gn_args" is missing or not an object',
                    self._problems()[0],
                )

    def test_args_file_must_be_source_absolute(self):
        write_builder(
            self.output_dir,
            'b',
            gn_args={**testing_support.GN_ARGS, 'args_file': 'build/x.gni'},
        )
        self.assertIn('not a source-absolute', self._problems()[0])

    def test_args_file_must_exist(self):
        src_dir = testing_support.use_src_dir(self)
        write_builder(
            self.output_dir,
            'b',
            gn_args={**testing_support.GN_ARGS, 'args_file': '//build/x.gni'},
        )
        self.assertIn("'//build/x.gni' does not exist", self._problems()[0])
        (src_dir / 'build').mkdir()
        (src_dir / 'build' / 'x.gni').write_text('')
        self.assertEqual(self._problems(), [])

    def test_spec_must_be_the_builders(self):
        for properties_json, problem in (
            ({}, '"builder_ids" does not have \'b\''),
            (
                testing_support.properties('b', builder_ids=[]),
                '"builder_ids" does not have \'b\'',
            ),
            (
                testing_support.properties(
                    'other', builder_ids=[{'builder': 'b'}]
                ),
                '"builder_db" has no spec for \'b\'',
            ),
        ):
            with self.subTest(problem=problem):
                write_builder(
                    self.output_dir, 'b', properties_json=properties_json
                )
                self.assertIn(problem, self._problems()[0])

    def test_spec_must_carry_what_consumers_read(self):
        write_builder(
            self.output_dir,
            'b',
            properties_json=testing_support.properties(
                'b',
                {
                    'chromium_config': {'build_config': 'Release'},
                    'gclient_config': {'target_os': 'linux', 'target_cpu': [1]},
                },
            ),
        )
        problem = self._problems()[0]
        self.assertIn('has no target_platform', problem)
        self.assertIn('gclient_config.target_os" is not a list', problem)
        self.assertIn('gclient_config.target_cpu" is not a list', problem)

    def test_spec_without_gclient_config(self):
        write_builder(
            self.output_dir,
            'b',
            properties_json=testing_support.properties(
                'b', {'chromium_config': {'target_platform': 'linux'}}
            ),
        )
        self.assertIn('gclient_config" is not an object', self._problems()[0])

    def test_sync_lists_are_checked(self):
        write_builder(
            self.output_dir, 'b', sync={'target_os': 'linux', 'target_cpu': [1]}
        )
        problem = self._problems()[0]
        self.assertIn("'target_os' is not a list of strings", problem)
        self.assertIn("'target_cpu' is not a list of strings", problem)


if __name__ == '__main__':
    unittest.main()
