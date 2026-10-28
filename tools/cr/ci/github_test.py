#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import github  # noqa: E402  -- after _boot
from ci.github import Check, State  # noqa: E402  -- after _boot

_TEAMCITY = 'https://teamcity.brave.com:443/buildConfiguration/X_Aggregate/1'
_JENKINS = (
    'https://ci.brave.com/job/brave-core-build-pr-windows-x64/job/PR-1/2/'
)


class ParseRollupTest(unittest.TestCase):
    def test_statuses_and_check_runs(self):
        checks = github.parse_rollup(
            [
                {
                    '__typename': 'StatusContext',
                    'context': 'PR browser (android)',
                    'state': 'FAILURE',
                    'targetUrl': _TEAMCITY,
                },
                {
                    '__typename': 'StatusContext',
                    'context': 'continuous-integration/windows-x64/pr-head',
                    'state': 'PENDING',
                    'targetUrl': _JENKINS,
                },
                {
                    '__typename': 'CheckRun',
                    'name': 'Socket Security',
                    'status': 'COMPLETED',
                    'conclusion': 'NEUTRAL',
                    'detailsUrl': 'https://socket.dev',
                },
                {
                    '__typename': 'CheckRun',
                    'name': 'lint',
                    'status': 'IN_PROGRESS',
                    'conclusion': '',
                    'detailsUrl': '',
                },
            ]
        )
        self.assertEqual(
            [(c.name, c.state, c.system) for c in checks],
            [
                ('PR browser (android)', State.FAILURE, 'teamcity'),
                (
                    'continuous-integration/windows-x64/pr-head',
                    State.PENDING,
                    'jenkins',
                ),
                ('Socket Security', State.NEUTRAL, 'socket.dev'),
                ('lint', State.PENDING, ''),
            ],
        )

    def test_keeps_the_latest_entry_per_name(self):
        checks = github.parse_rollup(
            [
                {'context': 'a', 'state': 'FAILURE', 'targetUrl': 'u1'},
                {'context': 'a', 'state': 'SUCCESS', 'targetUrl': 'u2'},
            ]
        )
        self.assertEqual(checks, (Check('a', State.SUCCESS, 'u2'),))


class PrChecksTest(unittest.TestCase):
    def test_reads_the_pull_request(self):
        data = {
            'number': 41074,
            'headRefName': 'cr159',
            'headRefOid': 'a' * 40,
            'statusCheckRollup': [
                {'context': 'a', 'state': 'ERROR', 'targetUrl': _JENKINS}
            ],
        }
        with mock.patch.object(
            github.GhCli, 'view_pr', return_value=data
        ) as view_pr:
            checks = github.pr_checks('cr159')
        view_pr.assert_called_once_with(
            'cr159',
            repo='brave/brave-core',
            fields='number,headRefName,headRefOid,statusCheckRollup',
        )
        self.assertEqual(checks.number, 41074)
        self.assertEqual(checks.head, 'a' * 40)
        self.assertEqual(
            checks.in_state(State.FAILURE),
            [Check('a', State.FAILURE, _JENKINS)],
        )


if __name__ == '__main__':
    unittest.main()
