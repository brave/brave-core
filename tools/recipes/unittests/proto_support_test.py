#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the proto compilation/support machinery."""

# White-box tests: they exercise internals (`_check_package`,
# `_gather_proto_info`), so protected-access is intentional.
# pylint: disable=protected-access

from __future__ import annotations

import contextlib
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
from google.protobuf import descriptor_pb2

import engine
import proto_support


def _fake_pb2_body(package: str) -> str:
    """A minimal `*_pb2.py`-shaped module body carrying a package descriptor.

    `_check_package` extracts the package by parsing the `DESCRIPTOR = ...
    AddSerializedFile(b'...')` assignment that protoc emits, so we synthesize
    exactly that shape from a FileDescriptorProto.
    """
    fdp = descriptor_pb2.FileDescriptorProto(package=package)
    return f'DESCRIPTOR = _pool.AddSerializedFile({fdp.SerializeToString()!r})\n'


class IsMessageClassTest(unittest.TestCase):

    def test_message_class(self):
        # A real compiled message class (compile + put PB on sys.path once).
        engine._ensure_protos()
        # pylint: disable=import-outside-toplevel,import-error
        from PB.recipes.brave.toolchains.rust.package_rust import (
            InputProperties)
        self.assertTrue(proto_support.is_message_class(InputProperties))

    def test_non_message(self):
        self.assertFalse(proto_support.is_message_class(int))
        self.assertFalse(proto_support.is_message_class('not a class'))


class CheckPackageTest(unittest.TestCase):
    """_check_package validates the .proto package matches its location."""

    def test_recipe_package_matches_full_path(self):
        relpath_base = os.path.join('recipes', 'brave', 'foo', 'bar')
        body = _fake_pb2_body('recipes.brave.foo.bar')
        self.assertIsNone(proto_support._check_package(body, relpath_base))

    def test_recipe_package_mismatch_reports_error(self):
        relpath_base = os.path.join('recipes', 'brave', 'foo', 'bar')
        body = _fake_pb2_body('recipes.brave.wrong.bar')
        err = proto_support._check_package(body, relpath_base)
        self.assertIsNotNone(err)
        self.assertIn('bad package', err)

    def test_module_example_matches_full_path(self):
        relpath_base = os.path.join('recipe_modules', 'brave', 'mod',
                                    'examples', 'full')
        body = _fake_pb2_body('recipe_modules.brave.mod.examples.full')
        self.assertIsNone(proto_support._check_package(body, relpath_base))

    def test_module_non_test_drops_last_token(self):
        # Non-example/test module protos are packaged one token short.
        relpath_base = os.path.join('recipe_modules', 'brave', 'mod', 'props')
        body = _fake_pb2_body('recipe_modules.brave.mod')
        self.assertIsNone(proto_support._check_package(body, relpath_base))


class GatherProtoInfoTest(unittest.TestCase):
    """_gather_proto_info maps repo .proto files into the PB namespace."""

    def test_maps_recipe_and_module_namespaces(self):
        by_relpath = {
            info.relpath: info.dest_relpath
            for info in proto_support._gather_proto_info()
        }
        self.assertEqual(
            by_relpath.get('recipes/toolchains/rust/package_rust.proto'),
            'recipes/brave/toolchains/rust/package_rust.proto')
        self.assertEqual(
            by_relpath.get(
                'recipe_modules/chromium_checkout/examples/full.proto'),
            'recipe_modules/brave/chromium_checkout/examples/full.proto')


class FileChecksumTest(unittest.TestCase):
    """_file_checksum computes a git-blob-style sha1 and rejects empty files."""

    def test_matches_git_blob_hash(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'blob'
            path.write_bytes(b'hello')
            # `git hash-object` of the bytes "hello".
            self.assertEqual(proto_support._file_checksum(path),
                             'b6fc4c620b67d95f953a5c1c1230aaab5db5a1b0')

    def test_raises_on_empty_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'empty'
            path.write_bytes(b'')
            with self.assertRaises(ValueError):
                proto_support._file_checksum(path)


class RecompileTest(unittest.TestCase):
    """A changed .proto changes the digest, so ensure_compiled rebuilds."""

    def test_digest_changes_when_a_proto_changes(self):
        # `ensure_compiled` recompiles when the digest from `_gather_protos`
        # no longer matches the installed `PB/csum`, so editing a proto must
        # change that digest. Point the scan at a throwaway tree (no protoc run).
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'recipes').mkdir()
            proto = root / 'recipes' / 'x.proto'
            proto.write_text('syntax = "proto3";\n'
                             'package recipes.brave.x;\n'
                             'message M { string a = 1; }\n')
            with mock.patch.object(proto_support, 'RECIPES_ROOT', root):
                dgst_before, _ = proto_support._gather_protos()
                proto.write_text('syntax = "proto3";\n'
                                 'package recipes.brave.x;\n'
                                 'message M { string a = 1; string b = 2; }\n')
                dgst_after, _ = proto_support._gather_protos()
        self.assertNotEqual(dgst_before, dgst_after)


class CompileErrorTest(unittest.TestCase):
    """_compile_protos surfaces protoc failures with source-relative paths."""

    def test_bad_proto_syntax_is_reported(self):
        # Reuse the engine's installed protoc; only the compile step is tested.
        proto_package = proto_support.ensure_compiled()
        protoc = os.path.join(proto_package, 'protoc', 'bin', 'protoc')

        with tempfile.TemporaryDirectory() as tmp:
            proto_tree = os.path.join(tmp, 'tree')
            dest_relpath = 'recipes/brave/norp.proto'
            src = os.path.join(proto_tree, dest_relpath)
            os.makedirs(os.path.dirname(src))
            with open(src, 'w') as proto:
                proto.write('syntax = "proto3"; norp')

            argfile = os.path.join(tmp, 'argfile')
            with open(argfile, 'w') as f:
                f.write(dest_relpath + '\n')

            out = os.path.join(tmp, 'out')
            os.makedirs(out)

            stderr = io.StringIO()
            with self.assertRaises(SystemExit), \
                    contextlib.redirect_stderr(stderr):
                proto_support._compile_protos([(src, dest_relpath)],
                                              proto_tree, protoc, argfile, out)

            message = stderr.getvalue()
            self.assertIn('Error while compiling protobufs', message)
            # _rel_to_abs_replacer rewrote the dest-relative path in protoc's
            # output back to the original source location.
            self.assertIn(os.path.dirname(src), message)


if __name__ == '__main__':
    unittest.main()
