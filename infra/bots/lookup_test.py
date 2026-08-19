#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for lookup.py's own logic: `cmd_lookup()`'s use of
generated_output.OutputGenerator. Reading a generated `gn-args.json` and
rendering it as `args.gn` text is OutputGenerator's own logic, tested in
generated_output_test.py. `bots.py`'s CLI wiring for the `lookup` subcommand
(dispatch, missing-positional handling) is exercised in bots_test.py
instead."""

import argparse
import contextlib
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gen_paths
import generated_output
import lookup
from generated_output_test import _make_generated_output_dir


class CmdLookupTest(unittest.TestCase):

    def test_quiet_prints_only_args_gn(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        _make_generated_output_dir(tmp.name, ['b'])

        original = gen_paths.BUILDERS_OUTPUT_DIR
        gen_paths.BUILDERS_OUTPUT_DIR = Path(tmp.name)
        try:
            args = argparse.Namespace(builder='b', quiet=True)
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                ret = lookup.cmd_lookup(args)
        finally:
            gen_paths.BUILDERS_OUTPUT_DIR = original

        self.assertEqual(ret, 0)
        self.assertEqual(buf.getvalue(), 'is_asan = true\n')

    def test_unknown_builder_raises(self):
        args = argparse.Namespace(builder='does-not-exist', quiet=True)
        with self.assertRaises(generated_output.BotsError):
            lookup.cmd_lookup(args)


if __name__ == '__main__':
    unittest.main()
