#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for generated_output.py: reading a generated `gn-args.json` and
rendering it as `args.gn` text, shared by `lookup.py` and `gen.py`. Their own
CLI-level tests seed a generated output dir the same way and just check that
they delegate here correctly."""

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gen_paths
import generated_output


def _make_generated_output_dir(tmp_dir, builder_names):
    """Writes a minimal `gn-args.json` for each name under `tmp_dir`."""
    for name in builder_names:
        builder_dir = Path(tmp_dir) / name
        builder_dir.mkdir()
        (builder_dir / 'gn-args.json').write_text(json.dumps(
            {'gn_args': {
                'is_asan': True
            }}),
                                                  encoding='utf-8')


class ListGeneratedBuildersTest(unittest.TestCase):

    def test_no_output_dir_means_no_builders(self):
        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(
            tempfile.mkdtemp()) / 'does-not-exist'
        try:
            self.assertEqual(generated_output.list_generated_builders(), [])
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

    def test_lists_builders_sorted_by_name(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _make_generated_output_dir(tmp.name, ['z-builder', 'a-builder'])

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            self.assertEqual(generated_output.list_generated_builders(),
                             ['a-builder', 'z-builder'])
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original


class ReadGeneratedGnArgsTest(unittest.TestCase):

    def test_missing_builder_raises_bots_error(self):
        with self.assertRaises(generated_output.BotsError):
            generated_output.OutputGenerator(
                'no-such-builder').read_generated_gn_args()

    def test_missing_builder_hint_lists_available_builders(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _make_generated_output_dir(tmp.name, ['a-builder', 'b-builder'])

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            with self.assertRaises(generated_output.BotsError) as ctx:
                generated_output.OutputGenerator(
                    'no-such-builder').read_generated_gn_args()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertIn('a-builder', str(ctx.exception))
        self.assertIn('b-builder', str(ctx.exception))


class RenderArgsGnTest(unittest.TestCase):

    def test_plain_args_no_secrets(self):
        rendered = generated_output.OutputGenerator('b').render_args_gn(
            {'gn_args': {
                'is_asan': True
            }})
        self.assertEqual(rendered, 'is_asan = true\n')

    def test_secrets_add_import_line_only_not_values(self):
        rendered = generated_output.OutputGenerator(
            'linux-x64-asan-brave').render_args_gn({
                'gn_args': {
                    'target_os': 'linux'
                },
                'secrets': {
                    'brave_services_key': 'BRAVE_SERVICES_KEY'
                },
            })
        lines = rendered.split('\n')
        self.assertEqual(lines[0],
                         'import("//out/linux-x64-asan-brave/secrets.gni")')
        self.assertNotIn('BRAVE_SERVICES_KEY', rendered)
        self.assertNotIn('brave_services_key', rendered)

    def test_args_file_import_line(self):
        rendered = generated_output.OutputGenerator('b').render_args_gn({
            'gn_args': {
                'target_os': 'linux'
            },
            'args_file': '//build/args/chromeos.gni',
        })
        self.assertEqual(
            rendered, 'import("//build/args/chromeos.gni")\n'
            'target_os = "linux"\n')

    def test_keys_are_sorted(self):
        rendered = generated_output.OutputGenerator('b').render_args_gn(
            {'gn_args': {
                'z': True,
                'a': True
            }})
        self.assertEqual(rendered, 'a = true\nz = true\n')


if __name__ == '__main__':
    unittest.main()
