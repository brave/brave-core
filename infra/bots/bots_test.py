#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Thin integration tests for `bots.py`'s CLI wiring: dispatch to each
subcommand and their error paths. Each subcommand's own logic is covered by
its own <name>_test.py; `gen`'s dispatch test stubs out `gn gen` so that it
never shells out to a real `gn` binary."""

import contextlib
import io
import json
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bots
import gen
import testing_support
from testing_support import write_builder

# How `testing_support.GN_ARGS` render in an `args.gn`.
_ARGS_GN = 'target_cpu = "x64"\ntarget_os = "linux"\n'


class _BotsTestCase(unittest.TestCase):
    def setUp(self):
        self.output_dir = testing_support.use_output_dir(self)

    def _main(self, *argv):
        """Runs `bots.py` with *argv*, returning its exit code, stdout and
        stderr."""
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            try:
                ret = bots.main(list(argv))
            except SystemExit as e:
                ret = e.code
        return ret, out.getvalue(), err.getvalue()


class BuilderArgumentTest(_BotsTestCase):
    """The subcommands taking a builder: unknown and missing builders."""

    def test_unknown_builder_returns_1(self):
        for command in ('lookup', 'gen', 'describe'):
            with self.subTest(command=command):
                ret, _, err = self._main(command, 'does-not-exist')
                self.assertEqual(ret, 1)
                self.assertIn('does-not-exist', err)

    def test_missing_builder_lists_available_builders(self):
        write_builder(self.output_dir, 'a-builder')
        write_builder(self.output_dir, 'b-builder')
        for command in ('lookup', 'gen'):
            with self.subTest(command=command):
                ret, _, err = self._main(command)
                self.assertEqual(ret, 2)
                self.assertIn('available builders:\n  a-builder\n', err)

    def test_snapshot_usage_error_does_not_list_builders(self):
        # `snapshot` has nothing to do with a builder name, so its usage
        # errors should not carry `lookup`'s builder-listing behaviour.
        write_builder(self.output_dir, 'a-builder')
        ret, _, err = self._main('snapshot', 'unexpected-positional')
        self.assertEqual(ret, 2)
        self.assertNotIn('available builders', err)


class DispatchTest(_BotsTestCase):
    def setUp(self):
        super().setUp()
        write_builder(self.output_dir, 'b')

    def test_lookup(self):
        self.assertEqual(
            self._main('lookup', 'b', '--quiet'), (0, _ARGS_GN, '')
        )

    def test_gen(self):
        src_dir = testing_support.use_src_dir(self)
        original = gen.BuildDir._run_gn_gen
        gen.BuildDir._run_gn_gen = lambda build_dir: 0
        self.addCleanup(setattr, gen.BuildDir, '_run_gn_gen', original)
        self.assertEqual(self._main('gen', 'b')[0], 0)
        self.assertEqual(
            (src_dir / 'out' / 'b' / 'args.gn').read_text(), _ARGS_GN
        )

    def test_validate(self):
        ret, out, _ = self._main('validate')
        self.assertEqual(ret, 0)
        self.assertIn('looks ok', out)

    def test_validate_problems_return_1(self):
        write_builder(self.output_dir, 'c', omit=('targets.json',))
        ret, _, err = self._main('validate')
        self.assertEqual(ret, 1)
        self.assertIn('targets.json: missing', err)

    def test_describe_one_builder(self):
        ret, out, _ = self._main('describe', 'b', '--json')
        self.assertEqual(ret, 0)
        self.assertEqual(list(json.loads(out)), ['b'])

    def test_describe_every_builder(self):
        write_builder(self.output_dir, 'a')
        ret, out, _ = self._main('describe', '--json')
        self.assertEqual(ret, 0)
        self.assertEqual(sorted(json.loads(out)), ['a', 'b'])


if __name__ == '__main__':
    unittest.main()
