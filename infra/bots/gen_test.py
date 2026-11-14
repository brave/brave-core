#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for gen.py: rendering a builder's `args.gn`, writing its build
directory with its secrets, and running `gn gen` in it. `gn gen` itself
shells out to a real `gn` binary and is stubbed out here. `bots.py`'s CLI
wiring for the `gen` subcommand is exercised in bots_test.py instead."""

import argparse
import os
import stat
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gen
import generated_output
import testing_support
from testing_support import write_builder

_GN_ARGS = {'target_cpu': 'x64', 'target_os': 'linux'}

# How `_GN_ARGS` render in an `args.gn`.
_ARGS_GN = 'target_cpu = "x64"\ntarget_os = "linux"\n'

_SECRETS = {
    'other_secret_key': 'OTHER_SECRET_ENV_VAR',
    'fake_secret_key': 'FAKE_SECRET_ENV_VAR',
}


class _GenTestCase(unittest.TestCase):
    def setUp(self):
        self.output_dir = testing_support.use_output_dir(self)
        self.src_dir = testing_support.use_src_dir(self)
        testing_support.use_dotenv(
            self, 'fake_secret_key=abc123\nother_secret_key=def456\n'
        )

    def _write_builder(self, **gn_args_json):
        write_builder(
            self.output_dir,
            'b',
            gn_args={'gn_args': _GN_ARGS, **gn_args_json},
        )


class RenderArgsGnTest(_GenTestCase):
    def test_plain_args(self):
        write_builder(
            self.output_dir,
            'b',
            gn_args={'gn_args': {'z': True, 'a': True, **_GN_ARGS}},
        )
        self.assertEqual(
            gen.BuildDir('b').render_args_gn(),
            'a = true\n' + _ARGS_GN + 'z = true\n',
        )

    def test_secrets_add_the_import_line_only(self):
        self._write_builder(secrets=_SECRETS)
        args_gn = gen.BuildDir('b').render_args_gn()
        self.assertEqual(
            args_gn.splitlines()[0], 'import("//out/b/secrets.gni")'
        )
        self.assertNotIn('abc123', args_gn)
        self.assertNotIn('fake_secret_key', args_gn)

    def test_secrets_import_follows_the_build_directory(self):
        self._write_builder(secrets=_SECRETS)
        self.assertEqual(
            gen.BuildDir('b', self.src_dir / 'custom')
            .render_args_gn()
            .splitlines()[0],
            'import("//custom/secrets.gni")',
        )

    def test_args_file_import_line(self):
        (self.src_dir / 'build').mkdir()
        (self.src_dir / 'build' / 'x.gni').write_text('')
        self._write_builder(args_file='//build/x.gni')
        self.assertEqual(
            gen.BuildDir('b').render_args_gn(),
            'import("//build/x.gni")\n' + _ARGS_GN,
        )

    def test_unknown_builder_raises(self):
        with self.assertRaises(generated_output.BotsError):
            gen.BuildDir('b').render_args_gn()


class WriteTest(_GenTestCase):
    def test_writes_args_gn_into_out_builder(self):
        self._write_builder()
        args_gn = gen.BuildDir('b').write()
        out_dir = self.src_dir / 'out' / 'b'
        self.assertEqual(args_gn, _ARGS_GN)
        self.assertEqual((out_dir / 'args.gn').read_text(), args_gn)
        self.assertFalse((out_dir / 'secrets.gni').exists())

    def test_writes_secrets_with_their_real_values_sorted(self):
        self._write_builder(secrets=_SECRETS)
        gen.BuildDir('b').write()
        self.assertEqual(
            (self.src_dir / 'out' / 'b' / 'secrets.gni').read_text(),
            'fake_secret_key = "abc123"\nother_secret_key = "def456"\n',
        )

    # Windows' `chmod()` only maps the read-only bit, so a file written
    # there always reads back as 0o666.
    @unittest.skipIf(
        sys.platform == 'win32', 'POSIX permission bits are not supported'
    )
    def test_secrets_file_is_readable_only_by_owner(self):
        self._write_builder(secrets=_SECRETS)
        gen.BuildDir('b').write()
        secrets_gni = self.src_dir / 'out' / 'b' / 'secrets.gni'
        self.assertEqual(stat.S_IMODE(secrets_gni.stat().st_mode), 0o600)

    def test_missing_secret_writes_nothing(self):
        testing_support.use_dotenv(self, 'fake_secret_key=abc123\n')
        self._write_builder(secrets=_SECRETS)
        with self.assertRaisesRegex(
            generated_output.BotsError, 'other_secret_key'
        ):
            gen.BuildDir('b').write()
        self.assertFalse((self.src_dir / 'out' / 'b').exists())

    def test_creates_a_nested_out_dir(self):
        self._write_builder()
        out_dir = self.src_dir / 'nested' / 'out-dir'
        gen.BuildDir('b', out_dir).write()
        self.assertTrue((out_dir / 'args.gn').is_file())


class GenTest(_GenTestCase):
    def setUp(self):
        super().setUp()
        self.gn_gen_dirs = []
        original = gen.BuildDir._run_gn_gen
        gen.BuildDir._run_gn_gen = lambda build_dir: (
            self.gn_gen_dirs.append(build_dir.out_dir) or 0
        )
        self.addCleanup(setattr, gen.BuildDir, '_run_gn_gen', original)
        self._write_builder()

    def test_writes_then_runs_gn_gen_in_out_builder(self):
        self.assertEqual(gen.BuildDir('b').gen(), 0)
        out_dir = self.src_dir / 'out' / 'b'
        self.assertEqual(self.gn_gen_dirs, [out_dir])
        self.assertTrue((out_dir / 'args.gn').is_file())

    def test_out_dir_outside_src_raises(self):
        with self.assertRaisesRegex(
            generated_output.BotsError, 'not under the chromium checkout'
        ):
            gen.BuildDir('b', self.output_dir)

    def test_cmd_gen_resolves_out_dir(self):
        out_dir = self.src_dir / 'custom'
        gen.cmd_gen(argparse.Namespace(builder='b', out_dir=str(out_dir)))
        self.assertEqual(self.gn_gen_dirs, [out_dir])


if __name__ == '__main__':
    unittest.main()
