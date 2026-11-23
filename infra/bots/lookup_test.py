#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for lookup.py's own logic: `cmd_lookup()`'s output. Rendering the
`args.gn` is gen.py's, tested in gen_test.py. `bots.py`'s CLI wiring for the
`lookup` subcommand is exercised in bots_test.py instead."""

import argparse
import contextlib
import io
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import generated_output
import lookup
import testing_support


class CmdLookupTest(unittest.TestCase):
    def setUp(self):
        output_dir = testing_support.use_output_dir(self)
        testing_support.write_builder(output_dir, 'b')

    def _lookup(self, **kwargs):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            ret = lookup.cmd_lookup(argparse.Namespace(**kwargs))
        self.assertEqual(ret, 0)
        return buf.getvalue()

    def test_quiet_prints_only_args_gn(self):
        self.assertEqual(
            self._lookup(builder='b', quiet=True),
            'target_cpu = "x64"\ntarget_os = "linux"\n',
        )

    def test_says_where_it_would_write(self):
        self.assertIn(
            'to out/b/args.gn', self._lookup(builder='b', quiet=False)
        )

    def test_unknown_builder_raises(self):
        with self.assertRaises(generated_output.BotsError):
            lookup.cmd_lookup(argparse.Namespace(builder='nope', quiet=True))


if __name__ == '__main__':
    unittest.main()
