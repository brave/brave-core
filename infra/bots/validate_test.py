#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for validate.py's own logic: `_Validator.run()`'s checks over
`generated/builders/` and `cmd_validate()`'s success/failure paths.
`bots.py`'s CLI wiring for the `validate` subcommand is exercised in
bots_test.py instead."""

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

import gen_paths
import generated_output
import dotenv
import validate


def _patch_dotenv(test_case, contents: str) -> None:
    """Points `dotenv.DEFAULT_PATH` at a temp file with `contents` for
    the duration of `test_case`."""
    tmp = tempfile.TemporaryDirectory()
    test_case.addCleanup(tmp.cleanup)
    path = Path(tmp.name) / '.env'
    path.write_text(contents, encoding='utf-8')

    original = dotenv.DEFAULT_PATH
    dotenv.DEFAULT_PATH = path
    test_case.addCleanup(lambda: setattr(dotenv, 'DEFAULT_PATH', original))


def _write_builder(output_dir,
                   name,
                   *,
                   gn_args=None,
                   sync=None,
                   targets=None,
                   omit=()):
    """Writes a builder's three generated files under `output_dir`.

    `gn_args`/`sync`/`targets` default to minimal-but-valid payloads. Pass
    `omit`, a subset of `validate._REQUIRED_FILES`, to skip writing specific
    files, for testing the missing-file path.
    """
    builder_dir = Path(output_dir) / name
    builder_dir.mkdir(parents=True, exist_ok=True)
    payloads = {
        'gn-args.json': gn_args if gn_args is not None else {
            'gn_args': {
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        },
        'sync.json': sync if sync is not None else {
            'target_os': 'linux',
            'target_cpu': 'x64',
            'gclient_overrides': {},
        },
        'targets.json': targets if targets is not None else {
            'compile': ['brave:all'],
            'tests': ['a_test'],
        },
    }
    for filename, payload in payloads.items():
        if filename in omit:
            continue
        (builder_dir / filename).write_text(json.dumps(payload),
                                            encoding='utf-8')


class _OutputDirTestCase(unittest.TestCase):
    """Points `gen_paths.BUILDERS_OUTPUT_DIR` at a scratch directory for the
    duration of each test."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.output_dir = Path(tmp.name)

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = self.output_dir
        self.addCleanup(setattr, gen_paths, 'BUILDERS_OUTPUT_DIR', original)


class ValidatorRunTest(_OutputDirTestCase):

    def test_no_generated_builders_says_run_snapshot_first(self):
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('bots.py snapshot', errs[0])

    def test_single_valid_builder_has_no_errors(self):
        _write_builder(self.output_dir, 'linux-x64-asan-brave')
        self.assertEqual(validate._Validator().run(), [])

    def test_two_builders_sharing_gn_args_but_not_targets_is_fine(self):
        # The two real ASan builders resolve to identical gn_args by design
        # and differ only in targets.json.
        _write_builder(self.output_dir,
                       'linux-x64-asan-brave',
                       targets={
                           'compile': ['brave:all'],
                           'tests': ['brave_all_unit_tests'],
                       })
        _write_builder(self.output_dir,
                       'linux-x64-asan-chromium',
                       targets={
                           'compile': ['brave:all'],
                           'tests': ['chromium_unit_tests'],
                       })
        self.assertEqual(validate._Validator().run(), [])

    def test_missing_file_is_reported(self):
        _write_builder(self.output_dir, 'b', omit=('targets.json', ))
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('targets.json', errs[0])
        self.assertIn('missing', errs[0])

    def test_invalid_json_is_reported(self):
        _write_builder(self.output_dir, 'b')
        (self.output_dir / 'b' / 'gn-args.json').write_text('not json',
                                                            encoding='utf-8')
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('gn-args.json', errs[0])
        self.assertIn('invalid JSON', errs[0])

    def test_gn_args_missing_target_os_is_reported(self):
        _write_builder(self.output_dir,
                       'b',
                       gn_args={'gn_args': {
                           'target_cpu': 'x64'
                       }})
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn("'target_os'", errs[0])

    def test_gn_args_not_an_object_is_reported(self):
        _write_builder(self.output_dir, 'b', gn_args={'gn_args': 'nope'})
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('not an object', errs[0])

    def test_gn_args_json_that_parses_to_an_empty_object_is_reported(self):
        # A JSON body of exactly `{}` is still a valid, parsed object -
        # falling back to truthiness instead of an explicit `is not None`
        # check would skip validating it entirely.
        _write_builder(self.output_dir, 'b', gn_args={})
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('missing or not an object', errs[0])

    def test_args_file_missing_on_disk_is_reported(self):
        _write_builder(self.output_dir,
                       'b',
                       gn_args={
                           'gn_args': {
                               'target_os': 'linux',
                               'target_cpu': 'x64',
                           },
                           'args_file': '//build/args/does_not_exist.gni',
                       })
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('does_not_exist.gni', errs[0])
        self.assertIn('does not exist', errs[0])

    def test_args_file_present_on_disk_is_fine(self):
        original_src_dir = validate._CHROMIUM_SRC_DIR
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        args_file_path = Path(fake_src_root.name) / 'build' / 'args' / 'x.gni'
        args_file_path.parent.mkdir(parents=True)
        args_file_path.write_text('', encoding='utf-8')
        validate._CHROMIUM_SRC_DIR = Path(fake_src_root.name)
        self.addCleanup(setattr, validate, '_CHROMIUM_SRC_DIR',
                        original_src_dir)

        _write_builder(self.output_dir,
                       'b',
                       gn_args={
                           'gn_args': {
                               'target_os': 'linux',
                               'target_cpu': 'x64',
                           },
                           'args_file': '//build/args/x.gni',
                       })
        self.assertEqual(validate._Validator().run(), [])

    def test_args_file_not_source_absolute_is_reported(self):
        _write_builder(self.output_dir,
                       'b',
                       gn_args={
                           'gn_args': {
                               'target_os': 'linux',
                               'target_cpu': 'x64',
                           },
                           'args_file': 'build/args/x.gni',
                       })
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('source-absolute', errs[0])

    def test_leaked_secret_value_is_reported(self):
        _patch_dotenv(self, 'fake_secret_key=super-secret-value\n')
        _write_builder(self.output_dir,
                       'b',
                       gn_args={
                           'gn_args': {
                               'target_os': 'linux',
                               'target_cpu': 'x64',
                               'unrelated_gn_arg': 'super-secret-value',
                           },
                           'secrets': {
                               'fake_secret_key': 'FAKE_SECRET_ENV_VAR',
                           },
                       })
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('unrelated_gn_arg', errs[0])
        self.assertIn('fake_secret_key', errs[0])
        self.assertNotIn('super-secret-value', errs[0])

    def test_unset_dummy_placeholder_is_not_flagged_as_leaked(self):
        # An unset secret and an unrelated gn_arg can both legitimately be
        # "dummy", and that coincidence must not read as one leaking into the
        # other.
        _patch_dotenv(self, 'fake_secret_key=dummy\n')
        _write_builder(self.output_dir,
                       'b',
                       gn_args={
                           'gn_args': {
                               'target_os': 'linux',
                               'target_cpu': 'x64',
                               'unrelated_gn_arg': 'dummy',
                           },
                           'secrets': {
                               'fake_secret_key': 'FAKE_SECRET_ENV_VAR',
                           },
                       })
        self.assertEqual(validate._Validator().run(), [])

    def test_secret_declared_but_not_in_dotenv_is_fine(self):
        _patch_dotenv(self, '')  # No matching entry.
        _write_builder(self.output_dir,
                       'b',
                       gn_args={
                           'gn_args': {
                               'target_os': 'linux',
                               'target_cpu': 'x64',
                           },
                           'secrets': {
                               'fake_secret_key': 'FAKE_SECRET_ENV_VAR',
                           },
                       })
        self.assertEqual(validate._Validator().run(), [])

    def test_sync_json_missing_field_is_reported(self):
        _write_builder(self.output_dir, 'b', sync={'target_os': 'linux'})
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 2)
        self.assertTrue(
            any("'target_cpu'" in e for e in errs)
            and any("'gclient_overrides'" in e for e in errs))

    def test_targets_json_field_not_a_list_is_reported(self):
        _write_builder(self.output_dir,
                       'b',
                       targets={
                           'compile': 'brave:all',
                           'tests': [],
                       })
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn("'compile'", errs[0])
        self.assertIn('not a list', errs[0])

    def test_exact_duplicate_builders_are_reported(self):
        _write_builder(self.output_dir, 'b1')
        _write_builder(self.output_dir, 'b2')
        errs = validate._Validator().run()
        self.assertEqual(len(errs), 1)
        self.assertIn('b1', errs[0])
        self.assertIn('b2', errs[0])
        self.assertIn('duplicates', errs[0])


class CmdValidateTest(_OutputDirTestCase):

    def test_returns_0_and_prints_ok_message(self):
        _write_builder(self.output_dir, 'linux-x64-asan-brave')
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            ret = validate.cmd_validate(argparse.Namespace(quiet=False))
        self.assertEqual(ret, 0)
        self.assertIn('looks ok', buf.getvalue())
        self.assertIn('1 builder', buf.getvalue())

    def test_quiet_suppresses_success_message(self):
        _write_builder(self.output_dir, 'b')
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            ret = validate.cmd_validate(argparse.Namespace(quiet=True))
        self.assertEqual(ret, 0)
        self.assertEqual(buf.getvalue(), '')

    def test_raises_bots_error_with_every_problem_when_invalid(self):
        _write_builder(self.output_dir, 'b1')
        _write_builder(self.output_dir, 'b2')
        with self.assertRaises(generated_output.BotsError) as ctx:
            validate.cmd_validate(argparse.Namespace(quiet=False))
        self.assertIn('duplicates', str(ctx.exception))


if __name__ == '__main__':
    unittest.main()
