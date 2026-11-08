#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import systems  # noqa: E402  -- after _boot
from ci.jenkins import JenkinsCi  # noqa: E402  -- after _boot

_JENKINS = 'https://ci.brave.com/job/brave-core-build-pr-linux-x64/job/PR-1/2/'
_TEAMCITY = 'https://teamcity.brave.com/buildConfiguration/X_Aggregate/7'
_ACTIONS = 'https://github.com/brave/brave-core/actions/runs/3/job/4'


class NameTest(unittest.TestCase):
    def test_names_the_systems_read_and_the_host_of_others(self):
        self.assertEqual(systems.name(_JENKINS), 'jenkins')
        self.assertEqual(systems.name(_TEAMCITY), 'teamcity')
        self.assertEqual(systems.name(_ACTIONS), 'actions')
        self.assertEqual(systems.name('https://github.com/brave'), 'github.com')
        self.assertEqual(systems.name('https://socket.dev/x'), 'socket.dev')


class DispatchTest(unittest.TestCase):
    def setUp(self):
        self.jenkins = mock.create_autospec(JenkinsCi, instance=True)
        self.teamcity = mock.Mock()
        self.made = []

        def make(client):
            def factory():
                self.made.append(client)
                return client

            return factory

        self.enterContext(
            mock.patch.dict(
                systems._CLIENTS,
                {
                    'jenkins': make(self.jenkins),
                    'teamcity': make(self.teamcity),
                },
            )
        )
        systems._client.cache_clear()
        self.addCleanup(systems._client.cache_clear)

    def test_reads_through_the_url_s_system_made_once(self):
        self.teamcity.failed_builds.return_value = ['build']
        self.assertEqual(systems.failed_builds(_TEAMCITY), ['build'])
        systems.full_log(_TEAMCITY)
        self.teamcity.full_log.assert_called_once_with(_TEAMCITY)
        self.assertEqual(self.made, [self.teamcity])

    def test_refuses_other_urls(self):
        with self.assertRaisesRegex(systems.Unsupported, 'Not a Jenkins'):
            systems.full_log('https://socket.dev/x')

    def test_only_jenkins_wipes_the_workspace(self):
        self.jenkins.rerun.return_value = 'Queued'
        self.assertEqual(systems.rerun(_JENKINS, wipe_workspace=True), 'Queued')
        self.jenkins.rerun.assert_called_once_with(
            _JENKINS, wipe_workspace=True
        )
        with self.assertRaisesRegex(systems.Unsupported, 'Jenkins only'):
            systems.rerun(_TEAMCITY, wipe_workspace=True)
        self.teamcity.rerun.assert_not_called()
        systems.rerun(_TEAMCITY)
        self.teamcity.rerun.assert_called_once_with(_TEAMCITY)


if __name__ == '__main__':
    unittest.main()
