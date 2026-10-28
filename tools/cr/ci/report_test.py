#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import report  # noqa: E402  -- after _boot
from ci.failures import FailedBuild, Kind  # noqa: E402  -- after _boot
from ci.github import Check, PullRequestChecks, State  # noqa: E402

_TEAMCITY = 'https://teamcity.brave.com:443/buildConfiguration/X_Aggregate/7'
_JENKINS = (
    'https://ci.brave.com/job/brave-core-build-pr-windows-x64/job/PR-1/2/'
)
_BUILD = FailedBuild('X_Build', 'https://tc/8', 'Exit code 1', Kind.BUILD)


class CollectTest(unittest.TestCase):
    def test_reads_each_failed_check_from_its_system(self):
        checks = PullRequestChecks(
            number=1,
            branch='cr159',
            head='a' * 40,
            checks=(
                Check('tc', State.FAILURE, _TEAMCITY),
                Check('jenkins', State.FAILURE, _JENKINS),
                Check('socket', State.FAILURE, 'https://socket.dev'),
                Check('green', State.SUCCESS, _TEAMCITY),
                Check('running', State.PENDING, _JENKINS),
            ),
        )
        teamcity = mock.Mock()
        teamcity.failed_builds.return_value = [_BUILD]
        jenkins = mock.Mock()
        jenkins.failed_builds.side_effect = RuntimeError('no access')

        found = report.collect(
            checks, jenkins=lambda: jenkins, teamcity=lambda: teamcity
        )

        teamcity.failed_builds.assert_called_once_with(7)
        jenkins.failed_builds.assert_called_once_with(_JENKINS)
        self.assertEqual([c.check for c in found], ['tc', 'jenkins', 'socket'])
        self.assertEqual(found[0].builds, (_BUILD,))
        self.assertEqual(found[1].error, 'RuntimeError: no access')
        self.assertEqual(found[2].error, 'socket.dev checks are not read')


if __name__ == '__main__':
    unittest.main()
