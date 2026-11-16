#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for describe.py's own logic: rendering a builder's generated output
as a human report or as JSON. Reading it is generated_output.py's, tested in
generated_output_test.py. `bots.py`'s CLI wiring for the `describe`
subcommand is exercised in bots_test.py instead."""

import argparse
import contextlib
import io
import json
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import describe
import generated_output
import testing_support
from testing_support import write_builder


class CmdDescribeTest(unittest.TestCase):
    def setUp(self):
        self.output_dir = testing_support.use_output_dir(self)

    def _describe(self, builder=None, json_output=False):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            ret = describe.cmd_describe(
                argparse.Namespace(builder=builder, json=json_output)
            )
        self.assertEqual(ret, 0)
        return buf.getvalue()

    def test_json_prints_the_builders_output(self):
        write_builder(self.output_dir, 'b')
        self.assertEqual(
            json.loads(self._describe('b', json_output=True)),
            {
                'b': {
                    'gn_args': testing_support.GN_ARGS,
                    'builder_spec': testing_support.BUILDER_SPEC,
                    'sync': testing_support.SYNC,
                    'targets': testing_support.TARGETS,
                }
            },
        )

    def test_no_builder_describes_every_builder_sorted(self):
        write_builder(self.output_dir, 'z-builder')
        write_builder(self.output_dir, 'a-builder')
        output = self._describe()
        self.assertLess(output.index('a-builder:'), output.index('z-builder:'))

    def test_no_builders_generated_raises(self):
        with self.assertRaises(generated_output.BotsError):
            self._describe()

    def test_unknown_builder_raises(self):
        with self.assertRaises(generated_output.BotsError):
            self._describe('nope')

    def test_human_report_never_prints_secret_values(self):
        testing_support.use_dotenv(self, 'brave_services_key=super-secret\n')
        write_builder(
            self.output_dir,
            'b',
            gn_args={
                **testing_support.GN_ARGS,
                'secrets': {'brave_services_key': 'BRAVE_SERVICES_KEY'},
            },
        )
        output = self._describe('b')
        self.assertIn('secrets: brave_services_key', output)
        self.assertNotIn('super-secret', output)

    def test_human_report_separates_build_and_sync_targets(self):
        write_builder(
            self.output_dir,
            'b',
            gn_args={
                'gn_args': {'target_os': 'android', 'target_cpu': 'arm64'}
            },
            properties_json=testing_support.properties(
                'b',
                {
                    'chromium_config': {
                        'target_platform': 'android',
                        'build_config': 'Release',
                    },
                    'gclient_config': {
                        'target_os': ['android'],
                        'target_cpu': [],
                    },
                },
            ),
            sync={'target_os': ['android', 'linux'], 'target_cpu': []},
        )
        output = self._describe('b')
        self.assertIn('  target_os:  android\n', output)
        self.assertIn('  target_cpu: arm64\n', output)
        self.assertIn('  build_config: Release\n', output)
        self.assertIn('  sync target_os:  android, linux\n', output)
        self.assertIn('  sync target_cpu: (host)\n', output)


if __name__ == '__main__':
    unittest.main()
