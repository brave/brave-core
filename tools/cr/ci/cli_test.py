#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import cli, systems  # noqa: E402  -- after _boot

_URL = 'https://ci.brave.com/job/brave-core-build-pr-linux-x64/job/PR-1/2/'


def _main(*args: str) -> tuple[int, str]:
    out = io.StringIO()
    with mock.patch('sys.argv', ['cli.py', *args]):
        with contextlib.redirect_stdout(out):
            result = cli.main()
    return result, out.getvalue()


class LogTest(unittest.TestCase):
    def _log(self, *args: str) -> tuple[int, str, str, mock.Mock]:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'build.log'
            with mock.patch.object(
                systems, 'full_log', return_value='a\nb\n'
            ) as full_log:
                result, out = _main('log', _URL, '--output', str(output), *args)
            text = output.read_text()
        return result, text, out, full_log

    def test_writes_the_log_with_its_timestamps(self):
        result, text, out, full_log = self._log()
        full_log.assert_called_once_with(_URL, timestamps=True)
        self.assertEqual((result, text), (0, 'a\nb\n'))
        self.assertIn('2 lines written', out)

    def test_leaves_the_timestamps_out_when_asked(self):
        _, _, _, full_log = self._log('--no-timestamps')
        full_log.assert_called_once_with(_URL, timestamps=False)

    def test_refuses_other_urls(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'build.log'
            result, out = _main(
                'log', 'https://example.com/1', '--output', str(output)
            )
            self.assertFalse(output.exists())
        self.assertEqual(result, 1)
        self.assertIn('Not a Jenkins, TeamCity or GitHub Actions build', out)


class RerunTest(unittest.TestCase):
    def test_prints_what_was_queued(self):
        with mock.patch.object(
            systems, 'rerun', return_value='Queued it again.'
        ) as rerun:
            result, out = _main('rerun', _URL, '--wipe-workspace')
        rerun.assert_called_once_with(_URL, wipe_workspace=True)
        self.assertEqual((result, out), (0, 'Queued it again.\n'))


if __name__ == '__main__':
    unittest.main()
