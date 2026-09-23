#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for describe.py's own logic: reading/merging a builder's generated
files and rendering them as a human report or as JSON. `bots.py`'s CLI
wiring for the `describe` subcommand is exercised in bots_test.py instead."""

import argparse
import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import describe
import gen_paths
import generated_output


def _write_builder(tmp_dir,
                   name,
                   *,
                   gn_args=None,
                   secrets=None,
                   sync=None,
                   targets=None):
    """Writes one builder's generated files under `tmp_dir`.

    Only `gn-args.json` is required by `describe_builder()`; `sync.json` and
    `targets.json` are written only when given, so tests can exercise a
    builder with just a `gn-args.json` the way `validate_test.py` does.
    """
    builder_dir = Path(tmp_dir) / name
    builder_dir.mkdir()

    gn_args_json = {'gn_args': gn_args if gn_args is not None else {}}
    if secrets is not None:
        gn_args_json['secrets'] = secrets
    (builder_dir / 'gn-args.json').write_text(json.dumps(gn_args_json),
                                              encoding='utf-8')

    if sync is not None:
        (builder_dir / 'sync.json').write_text(json.dumps(sync),
                                               encoding='utf-8')
    if targets is not None:
        (builder_dir / 'targets.json').write_text(json.dumps(targets),
                                                  encoding='utf-8')


class DescribeBuilderTest(unittest.TestCase):

    def test_missing_builder_raises_bots_error(self):
        with self.assertRaises(generated_output.BotsError):
            describe.describe_builder('no-such-builder')

    def test_merges_all_three_generated_files(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _write_builder(tmp.name,
                       'b',
                       gn_args={
                           'target_os': 'linux',
                           'target_cpu': 'x64'
                       },
                       secrets={'brave_services_key': 'BRAVE_SERVICES_KEY'},
                       sync={
                           'target_os': 'linux',
                           'target_cpu': 'x64',
                           'gclient_overrides': {}
                       },
                       targets={
                           'compile': ['brave:all'],
                           'tests': ['brave_browser_tests']
                       })

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            result = describe.describe_builder('b')
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(result['gn_args']['gn_args']['target_os'], 'linux')
        self.assertEqual(result['gn_args']['secrets'],
                         {'brave_services_key': 'BRAVE_SERVICES_KEY'})
        self.assertEqual(result['sync']['target_cpu'], 'x64')
        self.assertEqual(result['targets']['tests'], ['brave_browser_tests'])

    def test_sync_and_targets_are_optional(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _write_builder(tmp.name, 'b', gn_args={'target_os': 'linux'})

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            result = describe.describe_builder('b')
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertNotIn('sync', result)
        self.assertNotIn('targets', result)


class CmdDescribeTest(unittest.TestCase):

    def test_json_prints_json_for_one_builder(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _write_builder(tmp.name, 'b', gn_args={'target_os': 'linux'})

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            args = argparse.Namespace(builder='b', json=True)
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                ret = describe.cmd_describe(args)
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(ret, 0)
        parsed = json.loads(buf.getvalue())
        self.assertEqual(
            parsed, {'b': {
                'gn_args': {
                    'gn_args': {
                        'target_os': 'linux'
                    }
                }
            }})

    def test_no_builder_describes_every_builder_sorted(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _write_builder(tmp.name, 'z-builder', gn_args={'target_os': 'linux'})
        _write_builder(tmp.name, 'a-builder', gn_args={'target_os': 'linux'})

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            args = argparse.Namespace(builder=None, json=False)
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                ret = describe.cmd_describe(args)
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(ret, 0)
        output = buf.getvalue()
        self.assertLess(output.index('a-builder:'), output.index('z-builder:'))

    def test_no_builders_generated_raises(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            args = argparse.Namespace(builder=None, json=False)
            with self.assertRaises(generated_output.BotsError):
                describe.cmd_describe(args)
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

    def test_human_report_never_prints_secret_values(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _write_builder(
            tmp.name,
            'b',
            gn_args={
                'target_os': 'linux',
                'target_cpu': 'x64'
            },
            secrets={'brave_services_key': 'BRAVE_SERVICES_KEY'},
            targets={
                'compile': ['brave:all'],
                'tests': []
            },
        )

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        os.environ['BRAVE_SERVICES_KEY'] = 'super-secret-value'
        try:
            args = argparse.Namespace(builder='b', json=False)
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                ret = describe.cmd_describe(args)
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original
            del os.environ['BRAVE_SERVICES_KEY']

        self.assertEqual(ret, 0)
        output = buf.getvalue()
        self.assertIn('brave_services_key', output)
        self.assertNotIn('super-secret-value', output)


if __name__ == '__main__':
    unittest.main()
