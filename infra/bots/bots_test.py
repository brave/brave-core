#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Thin integration tests for `bots.py`'s CLI wiring: dispatch to the
`snapshot`/`lookup`/`gen` subcommands and their error paths. Each
subcommand's own logic is covered by its own <name>_test.py; `gen`'s
dispatch test stubs out `gen.BuildDirGenerator.run_gn_gen()` so it never
shells out to a real `gn` binary."""

import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(
    0,
    os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                 'config'))

import bots
import gen
import gen_paths
import lookup


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


class LookupDispatchTest(unittest.TestCase):
    """`bots.py lookup`'s CLI wiring: dispatch to `lookup.cmd_lookup()` and
    the missing-positional/unknown-builder error paths. See lookup_test.py
    for `lookup.py`'s own logic."""

    def test_dispatches_to_lookup_cmd(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _make_generated_output_dir(tmp.name, ['b'])

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                ret = bots.main(['lookup', 'b', '--quiet'])
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(ret, 0)
        self.assertEqual(buf.getvalue(), 'is_asan = true\n')

    def test_unknown_builder_returns_1(self):
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            ret = bots.main(['lookup', 'does-not-exist', '--quiet'])
        self.assertEqual(ret, 1)
        self.assertIn('does-not-exist', buf.getvalue())

    def test_missing_positional_lists_available_builders(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _make_generated_output_dir(tmp.name, ['a-builder', 'b-builder'])

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            buf = io.StringIO()
            with contextlib.redirect_stderr(buf):
                with self.assertRaises(SystemExit) as ctx:
                    bots.main(['lookup'])
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(ctx.exception.code, 2)
        self.assertIn('a-builder', buf.getvalue())
        self.assertIn('b-builder', buf.getvalue())


class SnapshotDispatchTest(unittest.TestCase):

    def test_usage_error_does_not_list_builders(self):
        # `snapshot` has nothing to do with a builder name, so its usage
        # errors should not carry `lookup`'s builder-listing behaviour.
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _make_generated_output_dir(tmp.name, ['a-builder'])

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            buf = io.StringIO()
            with contextlib.redirect_stderr(buf):
                with self.assertRaises(SystemExit) as ctx:
                    bots.main(['snapshot', 'unexpected-positional'])
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(ctx.exception.code, 2)
        self.assertNotIn('a-builder', buf.getvalue())
        self.assertNotIn('available builders', buf.getvalue())


class GenDispatchTest(unittest.TestCase):
    """`bots.py gen`'s CLI wiring: dispatch to `gen.cmd_gen()` and the
    missing-positional/unknown-builder error paths. See gen_test.py for
    `gen.py`'s own logic (writing args.gn, the secrets stub)."""

    def test_dispatches_to_gen_cmd(self):
        generated_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(generated_tmp.cleanup)
        _make_generated_output_dir(generated_tmp.name, ['b'])
        # `_CHROMIUM_SRC_DIR` is faked so an --out-dir under it passes
        # cmd_gen()'s "must be inside the source root" check without
        # touching the real checkout.
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        out_dir = Path(fake_src_root.name) / 'out' / 'b'

        original_output_dir = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(generated_tmp.name)
        original_src_dir = gen._CHROMIUM_SRC_DIR
        gen._CHROMIUM_SRC_DIR = Path(fake_src_root.name)
        original_run_gn_gen = gen.BuildDirGenerator.run_gn_gen
        gen.BuildDirGenerator.run_gn_gen = lambda self: 0
        try:
            ret = bots.main(['gen', 'b', '--out-dir', str(out_dir)])
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original_output_dir
            gen._CHROMIUM_SRC_DIR = original_src_dir
            gen.BuildDirGenerator.run_gn_gen = original_run_gn_gen

        self.assertEqual(ret, 0)
        self.assertEqual((out_dir / 'args.gn').read_text(encoding='utf-8'),
                         'is_asan = true\n')

    def test_unknown_builder_returns_1(self):
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            ret = bots.main(['gen', 'does-not-exist'])
        self.assertEqual(ret, 1)
        self.assertIn('does-not-exist', buf.getvalue())

    def test_missing_positional_lists_available_builders(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _make_generated_output_dir(tmp.name, ['a-builder', 'b-builder'])

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            buf = io.StringIO()
            with contextlib.redirect_stderr(buf):
                with self.assertRaises(SystemExit) as ctx:
                    bots.main(['gen'])
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(ctx.exception.code, 2)
        self.assertIn('a-builder', buf.getvalue())
        self.assertIn('b-builder', buf.getvalue())


if __name__ == '__main__':
    unittest.main()
