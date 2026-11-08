#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci.actions import ActionsCi  # noqa: E402  -- after _boot
from ci.failures import Kind  # noqa: E402  -- after _boot

_URL = 'https://github.com/brave/brave-core/actions/runs/41/job/125'
_JOB = {
    'id': 125,
    'name': 'security',
    'status': 'completed',
    'conclusion': 'failure',
    'html_url': _URL,
    'steps': [
        {'number': 1, 'name': 'Set up job', 'conclusion': 'success'},
        {'number': 2, 'name': 'Run checks', 'conclusion': 'failure'},
        {'number': 3, 'name': 'Post checkout', 'conclusion': 'success'},
    ],
}
_LOG = (
    '\N{ZERO WIDTH NO-BREAK SPACE}2026-11-03T13:26:55.28Z Set up\n'
    '2026-11-03T13:27:01.10Z Checking\n'
    "2026-11-03T13:27:02.20Z HttpError: 504 Gateway Timeout\n"
    '2026-11-03T13:27:02.30Z ##[error]Process completed with exit code 1.\n'
    '2026-11-03T13:27:03.00Z Post job cleanup.\n'
)


class ActionsCiTest(unittest.TestCase):
    def _ci(self) -> tuple[ActionsCi, mock.Mock]:
        gh = mock.Mock()
        gh.api.side_effect = lambda path, **_: (
            _LOG if path.endswith('/logs') else json.dumps(_JOB)
        )
        return ActionsCi(gh), gh

    def test_reads_the_failed_steps_and_the_log_up_to_the_error(self):
        ci, gh = self._ci()
        [build] = ci.failed_builds(_URL)
        gh.api.assert_any_call('repos/brave/brave-core/actions/jobs/125')
        gh.api.assert_any_call('repos/brave/brave-core/actions/jobs/125/logs')
        self.assertEqual(build.name, 'security')
        self.assertEqual(build.url, _URL)
        self.assertEqual(build.summary, 'failure in Run checks')
        self.assertEqual(build.problems, ('Step 2, Run checks: failure',))
        self.assertEqual(build.kind, Kind.BUILD)
        self.assertEqual(
            build.log_tail,
            'Set up\nChecking\nHttpError: 504 Gateway Timeout\n'
            '##[error]Process completed with exit code 1.',
        )

    def test_keeps_the_log_end_without_an_error_line(self):
        self.assertEqual(ActionsCi._error_tail('a\nb\n'), 'a\nb')

    def test_a_fatal_line_above_the_error_leads_the_tail(self):
        fatal = (
            'F1108 13:30:24.969724   11708 glog.go:261] log: exiting: '
            'There is not enough space on the disk.'
        )
        log = '\n'.join(
            [f'2026-11-03T13:27:00.00Z {fatal}']
            + ['2026-11-03T13:27:01.00Z spew'] * 100
            + ['2026-11-03T13:27:02.00Z ##[error]exit code 1.']
        )
        gh = mock.Mock()
        gh.api.side_effect = lambda path, **_: (
            log if path.endswith('/logs') else json.dumps(_JOB)
        )
        [build] = ActionsCi(gh).failed_builds(_URL)
        self.assertTrue(build.log_tail.startswith(f'{fatal}\n[...]\nspew\n'))
        self.assertEqual(build.kind, Kind.INFRA)

    def test_full_log_drops_the_bom_and_timestamps_if_asked(self):
        ci, _ = self._ci()
        self.assertEqual(
            ci.full_log(_URL, timestamps=False),
            'Set up\nChecking\nHttpError: 504 Gateway Timeout\n'
            '##[error]Process completed with exit code 1.\n'
            'Post job cleanup.\n',
        )
        self.assertEqual(
            ci.full_log(_URL),
            _LOG.removeprefix('\N{ZERO WIDTH NO-BREAK SPACE}'),
        )

    def test_reruns_the_job(self):
        ci, gh = self._ci()
        self.assertIn('125', ci.rerun(_URL))
        gh.api.assert_called_once_with(
            'repos/brave/brave-core/actions/jobs/125/rerun', method='POST'
        )

    def test_refuses_other_urls(self):
        ci, _ = self._ci()
        with self.assertRaises(ValueError):
            ci.rerun('https://ci.brave.com/job/x/1/')


if __name__ == '__main__':
    unittest.main()
