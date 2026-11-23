#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import systems  # noqa: E402  -- after _boot
from ci.actions import ActionsCi  # noqa: E402  -- after _boot
from ci.jenkins import JenkinsCi  # noqa: E402  -- after _boot
from ci.teamcity import TeamCityCi  # noqa: E402  -- after _boot

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
        self.teamcity = mock.create_autospec(TeamCityCi, instance=True)
        self.actions = mock.create_autospec(ActionsCi, instance=True)
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
                    'actions': make(self.actions),
                },
            )
        )
        systems._client.cache_clear()
        self.addCleanup(systems._client.cache_clear)

    def test_reads_through_the_url_s_system_made_once(self):
        self.teamcity.failed_builds.return_value = ['build']
        self.assertEqual(systems.failed_builds(_TEAMCITY), ['build'])
        systems.full_log(_TEAMCITY, timestamps=True)
        self.teamcity.full_log.assert_called_once_with(
            _TEAMCITY, timestamps=True
        )
        self.assertEqual(self.made, [self.teamcity])

    def test_refuses_other_urls(self):
        with self.assertRaisesRegex(systems.Unsupported, 'Not a Jenkins'):
            systems.full_log('https://socket.dev/x')

    def test_jenkins_and_teamcity_clean_the_workspace(self):
        self.jenkins.rerun.return_value = 'Queued'
        self.assertEqual(systems.rerun(_JENKINS, wipe_workspace=True), 'Queued')
        self.jenkins.rerun.assert_called_once_with(
            _JENKINS, wipe_workspace=True, remove_src_out=False
        )
        systems.rerun(_TEAMCITY, remove_src_out=True)
        self.teamcity.rerun.assert_called_once_with(
            _TEAMCITY, wipe_workspace=False, remove_src_out=True
        )

    def test_test_log_is_for_jenkins_only(self):
        self.jenkins.test_log.return_value = 'Suite.A:\nout'
        self.assertEqual(systems.test_log(_JENKINS, 'Suite.*'), 'Suite.A:\nout')
        self.jenkins.test_log.assert_called_once_with(_JENKINS, 'Suite.*')
        with self.assertRaisesRegex(systems.Unsupported, 'Jenkins only'):
            systems.test_log(_TEAMCITY, 'Suite.*')

    def test_github_actions_cleans_nothing(self):
        for flag in ('wipe_workspace', 'remove_src_out'):
            with self.assertRaisesRegex(
                systems.Unsupported, 'Jenkins and TeamCity only'
            ):
                systems.rerun(_ACTIONS, **{flag: True})
        self.actions.rerun.assert_not_called()
        systems.rerun(_ACTIONS)
        self.actions.rerun.assert_called_once_with(_ACTIONS)


if __name__ == '__main__':
    unittest.main()
