#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for gen.py's own logic: `BuildDirGenerator`'s secret resolution, its
writing of a build directory's `args.gn`/`secrets.gni`, and
`cmd_gen()`'s validation and dispatch. `BuildDirGenerator.run_gn_gen()`
shells out to a real `gn` binary and is exercised manually, not here;
`cmd_gen()`'s tests stub it out. `bots.py`'s CLI wiring for the `gen`
subcommand is exercised in bots_test.py instead."""

import argparse
import os
import stat
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gen
import gen_paths
import generated_output
import dotenv
from generated_output_test import _make_generated_output_dir


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


class ResolveSecretsTest(unittest.TestCase):

    def test_empty_secrets_resolves_empty(self):
        self.assertEqual(gen.BuildDirGenerator('b').resolve_secrets({}), {})

    def test_resolves_from_dotenv_by_gn_arg_name(self):
        _patch_dotenv(
            self, 'fake_secret_key=abc123\nother_fake_secret_key=def456\n')

        resolved = gen.BuildDirGenerator('b').resolve_secrets({
            'fake_secret_key': 'FAKE_SECRET_ENV_VAR',
            'other_fake_secret_key': 'OTHER_FAKE_SECRET_ENV_VAR',
        })

        self.assertEqual(resolved, {
            'fake_secret_key': 'abc123',
            'other_fake_secret_key': 'def456',
        })

    def test_missing_secret_raises(self):
        _patch_dotenv(self, '')

        with self.assertRaises(generated_output.BotsError) as ctx:
            gen.BuildDirGenerator('b').resolve_secrets(
                {'fake_secret_key': 'FAKE_SECRET_ENV_VAR'})

        self.assertIn('fake_secret_key', str(ctx.exception))


class RenderSecretsGniTest(unittest.TestCase):

    def test_empty_secrets_renders_empty(self):
        self.assertEqual(gen.BuildDirGenerator.render_secrets_gni({}), '')

    def test_renders_real_values(self):
        rendered = gen.BuildDirGenerator.render_secrets_gni({
            'fake_secret_key': 'abc123',
            'other_fake_secret_key': 'def456',
        })
        self.assertEqual(
            rendered, 'fake_secret_key = "abc123"\n'
            'other_fake_secret_key = "def456"\n')

    def test_keys_are_sorted(self):
        rendered = gen.BuildDirGenerator.render_secrets_gni({
            'z': 'z-value',
            'a': 'a-value'
        })
        self.assertEqual(rendered, 'a = "a-value"\nz = "z-value"\n')


class DefaultOutDirTest(unittest.TestCase):

    def test_is_out_slash_builder_name_under_src_root(self):
        generator = gen.BuildDirGenerator('linux-x64-asan-brave')
        self.assertEqual(
            generator.out_dir,
            gen._CHROMIUM_SRC_DIR / 'out' / 'linux-x64-asan-brave')


class WriteBuildDirTest(unittest.TestCase):

    def test_writes_args_gn(self):
        generated_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(generated_tmp.cleanup)
        _make_generated_output_dir(generated_tmp.name, ['b'])
        out_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(out_tmp.cleanup)

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(generated_tmp.name)
        try:
            generator = gen.BuildDirGenerator('b', Path(out_tmp.name))
            args_gn = generator.write_build_dir()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(args_gn, 'is_asan = true\n')
        self.assertEqual(
            (Path(out_tmp.name) / 'args.gn').read_text(encoding='utf-8'),
            'is_asan = true\n')

    def test_creates_out_dir_if_missing(self):
        generated_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(generated_tmp.cleanup)
        _make_generated_output_dir(generated_tmp.name, ['b'])
        out_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(out_tmp.cleanup)
        out_dir = Path(out_tmp.name) / 'nested' / 'out-dir'

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(generated_tmp.name)
        try:
            gen.BuildDirGenerator('b', out_dir).write_build_dir()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertTrue((out_dir / 'args.gn').is_file())

    def test_no_secrets_means_no_stub_file(self):
        generated_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(generated_tmp.cleanup)
        _make_generated_output_dir(generated_tmp.name, ['b'])
        out_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(out_tmp.cleanup)

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(generated_tmp.name)
        try:
            gen.BuildDirGenerator('b', Path(out_tmp.name)).write_build_dir()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertFalse((Path(out_tmp.name) / 'secrets.gni').is_file())

    def test_declared_secrets_get_their_real_value(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        builder_dir = Path(tmp.name) / 'b'
        builder_dir.mkdir()
        (builder_dir / 'gn-args.json').write_text(
            '{"gn_args": {"is_asan": true}, '
            '"secrets": {"fake_secret_key": "FAKE_SECRET_ENV_VAR"}}',
            encoding='utf-8')
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        fake_src_root_path = Path(fake_src_root.name).resolve()
        out_dir = fake_src_root_path / 'out' / 'b'
        _patch_dotenv(self, 'fake_secret_key=abc123\n')

        original_output_dir = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        original_src_dir = gen._CHROMIUM_SRC_DIR
        gen._CHROMIUM_SRC_DIR = fake_src_root_path
        try:
            gen.BuildDirGenerator('b', out_dir).write_build_dir()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original_output_dir
            gen._CHROMIUM_SRC_DIR = original_src_dir

        secrets_path = out_dir / 'secrets.gni'
        self.assertEqual(secrets_path.read_text(encoding='utf-8'),
                         'fake_secret_key = "abc123"\n')

    def test_secrets_file_is_readable_only_by_owner(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        builder_dir = Path(tmp.name) / 'b'
        builder_dir.mkdir()
        (builder_dir / 'gn-args.json').write_text(
            '{"gn_args": {"is_asan": true}, '
            '"secrets": {"fake_secret_key": "FAKE_SECRET_ENV_VAR"}}',
            encoding='utf-8')
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        fake_src_root_path = Path(fake_src_root.name).resolve()
        out_dir = fake_src_root_path / 'out' / 'b'
        _patch_dotenv(self, 'fake_secret_key=abc123\n')

        original_output_dir = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        original_src_dir = gen._CHROMIUM_SRC_DIR
        gen._CHROMIUM_SRC_DIR = fake_src_root_path
        try:
            gen.BuildDirGenerator('b', out_dir).write_build_dir()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original_output_dir
            gen._CHROMIUM_SRC_DIR = original_src_dir

        mode = stat.S_IMODE((out_dir / 'secrets.gni').stat().st_mode)
        self.assertEqual(mode, 0o600)

    def test_missing_declared_secret_raises(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        builder_dir = Path(tmp.name) / 'b'
        builder_dir.mkdir()
        (builder_dir / 'gn-args.json').write_text(
            '{"gn_args": {"is_asan": true}, '
            '"secrets": {"fake_secret_key": "FAKE_SECRET_ENV_VAR"}}',
            encoding='utf-8')
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        fake_src_root_path = Path(fake_src_root.name).resolve()
        out_dir = fake_src_root_path / 'out' / 'b'
        _patch_dotenv(self, '')  # No matching entry.

        original_output_dir = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        original_src_dir = gen._CHROMIUM_SRC_DIR
        gen._CHROMIUM_SRC_DIR = fake_src_root_path
        try:
            with self.assertRaises(generated_output.BotsError):
                gen.BuildDirGenerator('b', out_dir).write_build_dir()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original_output_dir
            gen._CHROMIUM_SRC_DIR = original_src_dir

        self.assertFalse((out_dir / 'args.gn').exists())

    def test_secrets_import_path_follows_a_non_default_out_dir(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        builder_dir = Path(tmp.name) / 'b'
        builder_dir.mkdir()
        (builder_dir / 'gn-args.json').write_text(
            '{"gn_args": {"is_asan": true}, '
            '"secrets": {"fake_secret_key": "FAKE_SECRET_ENV_VAR"}}',
            encoding='utf-8')
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        fake_src_root_path = Path(fake_src_root.name).resolve()
        # Deliberately not the default `out/b` layout.
        out_dir = fake_src_root_path / 'custom' / 'out-dir'
        _patch_dotenv(self, 'fake_secret_key=abc123\n')

        original_output_dir = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        original_src_dir = gen._CHROMIUM_SRC_DIR
        gen._CHROMIUM_SRC_DIR = fake_src_root_path
        try:
            args_gn = gen.BuildDirGenerator('b', out_dir).write_build_dir()
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original_output_dir
            gen._CHROMIUM_SRC_DIR = original_src_dir

        self.assertEqual(args_gn.splitlines()[0],
                         'import("//custom/out-dir/secrets.gni")')
        self.assertTrue((out_dir / 'secrets.gni').is_file())

    def test_unknown_builder_raises(self):
        generator = gen.BuildDirGenerator('no-such-builder',
                                          Path(tempfile.mkdtemp()))
        with self.assertRaises(generated_output.BotsError):
            generator.write_build_dir()


class CmdGenTest(unittest.TestCase):

    def test_out_dir_outside_src_root_raises(self):
        args = argparse.Namespace(builder='b', out_dir='/not/under/src/root')
        with self.assertRaises(generated_output.BotsError):
            gen.cmd_gen(args)

    def test_dispatches_to_gen(self):
        generated_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(generated_tmp.cleanup)
        _make_generated_output_dir(generated_tmp.name, ['b'])
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        fake_src_root_path = Path(fake_src_root.name).resolve()
        out_dir = fake_src_root_path / 'out' / 'b'

        seen_out_dirs = []
        original_run_gn_gen = gen.BuildDirGenerator.run_gn_gen
        gen.BuildDirGenerator.run_gn_gen = (
            lambda self: seen_out_dirs.append(self.out_dir) or 0)
        original_src_dir = gen._CHROMIUM_SRC_DIR
        gen._CHROMIUM_SRC_DIR = fake_src_root_path
        original_output_dir = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(generated_tmp.name)
        try:
            args = argparse.Namespace(builder='b', out_dir=str(out_dir))
            ret = gen.cmd_gen(args)
        finally:
            gen.BuildDirGenerator.run_gn_gen = original_run_gn_gen
            gen._CHROMIUM_SRC_DIR = original_src_dir
            gen_paths.BUILDERS_OUTPUT_DIR = original_output_dir

        self.assertEqual(ret, 0)
        self.assertEqual(seen_out_dirs, [out_dir])
        self.assertTrue((out_dir / 'args.gn').is_file())

    def test_default_out_dir_when_not_given(self):
        generated_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(generated_tmp.cleanup)
        _make_generated_output_dir(generated_tmp.name, ['b'])
        fake_src_root = tempfile.TemporaryDirectory()
        self.addCleanup(fake_src_root.cleanup)
        fake_src_root_path = Path(fake_src_root.name).resolve()

        seen_out_dirs = []
        original_run_gn_gen = gen.BuildDirGenerator.run_gn_gen
        gen.BuildDirGenerator.run_gn_gen = (
            lambda self: seen_out_dirs.append(self.out_dir) or 0)
        original_src_dir = gen._CHROMIUM_SRC_DIR
        gen._CHROMIUM_SRC_DIR = fake_src_root_path
        original_output_dir = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(generated_tmp.name)
        try:
            args = argparse.Namespace(builder='b', out_dir=None)
            gen.cmd_gen(args)
        finally:
            gen.BuildDirGenerator.run_gn_gen = original_run_gn_gen
            gen._CHROMIUM_SRC_DIR = original_src_dir
            gen_paths.BUILDERS_OUTPUT_DIR = original_output_dir

        self.assertEqual(seen_out_dirs, [fake_src_root_path / 'out' / 'b'])


if __name__ == '__main__':
    unittest.main()
