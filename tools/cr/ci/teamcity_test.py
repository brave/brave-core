#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import os
import subprocess
import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import teamcity  # noqa: E402  -- after _boot
from ci.failures import Kind  # noqa: E402  -- after _boot

_LOG = '\n'.join(
    [
        '[Step 1/3] gn gen out/android_Static',
        '[Step 1/3] Run gn clean before switching from ninja to siso in out',
        '[Step 1/3] Process exited with code 1',
        '[Step 3/3] cleanup',
    ]
)


class FakeCli:
    """Answers `teamcity api` paths from a table, and records each call."""

    def __init__(self, responses: dict[str, object]) -> None:
        self.responses = responses
        self.calls: list[tuple[list[str], dict]] = []

    def __call__(self, cmd, **kwargs):
        self.calls.append((cmd, kwargs['env']))
        if cmd[1] == 'api':
            path = cmd[-1]
            for prefix, response in self.responses.items():
                if path.startswith(prefix):
                    out = (
                        response
                        if isinstance(response, str)
                        else json.dumps(response)
                    )
                    return subprocess.CompletedProcess(cmd, 0, out, '')
            raise AssertionError(f'unexpected path {path}')
        return subprocess.CompletedProcess(cmd, 0, 'queued', '')


def _builds(*builds):
    return {'build': list(builds)}


class BuildIdTest(unittest.TestCase):
    def test_build_configuration_url(self):
        self.assertEqual(
            teamcity.build_id(
                'https://teamcity.brave.com:443/buildConfiguration/'
                'BraveBrowser_Android_Aggregate/753605'
            ),
            753605,
        )

    def test_view_log_url(self):
        self.assertEqual(
            teamcity.build_id('https://tc/viewLog.html?buildId=12&tab=log'), 12
        )

    def test_rejects_other_urls(self):
        with self.assertRaises(ValueError):
            teamcity.build_id('https://ci.brave.com/job/x/1/')


class FailedBuildsTest(unittest.TestCase):
    def test_reads_problems_tests_and_the_failed_step(self):
        cli = FakeCli(
            {
                '/app/rest/builds?locator=snapshotDependency:(to:(id:10)': (
                    _builds(
                        {
                            'id': 11,
                            'buildTypeId': 'Android_Build',
                            'statusText': 'Exit code 1',
                            'webUrl': 'https://tc/11',
                        },
                        {
                            'id': 12,
                            'buildTypeId': 'Android_Test',
                            'statusText': 'Tests failed: 1',
                            'webUrl': 'https://tc/12',
                        },
                        {
                            'id': 13,
                            'buildTypeId': 'Android_Upload',
                            'statusText': 'Snapshot dependency failed',
                            'webUrl': 'https://tc/13',
                        },
                    )
                ),
                '/app/rest/problemOccurrences?locator=build:(id:11)': {
                    'problemOccurrence': [
                        {
                            'type': 'TC_EXIT_CODE',
                            'details': 'Process exited with code 1',
                        }
                    ]
                },
                '/app/rest/problemOccurrences?locator=build:(id:12)': {
                    'problemOccurrence': [{'type': 'TC_FAILED_TESTS_bt1'}]
                },
                '/app/rest/problemOccurrences?locator=build:(id:13)': {
                    'problemOccurrence': [
                        {'type': 'TC_FAILED_SNAPSHOT_DEPENDENCY_ERROR'}
                    ]
                },
                '/app/rest/testOccurrences?locator=build:(id:11)': {},
                '/app/rest/testOccurrences?locator=build:(id:12)': {
                    'testOccurrence': [
                        {
                            'name': 'unit: Suite.Flaky',
                            'status': 'FAILURE',
                            'details': 'Expected 1',
                            'newFailure': False,
                            'firstFailed': {
                                'build': {'id': 5, 'webUrl': 'https://tc/5'}
                            },
                            'invocations': {'count': 2, 'failed': 2},
                        },
                        {
                            'name': 'unit: Suite.New',
                            'status': 'FAILURE',
                            'newFailure': True,
                        },
                    ]
                },
                '/downloadBuildLog.html?buildId=11': _LOG,
            }
        )
        builds = teamcity.TeamCityCi(run=cli).failed_builds(10)

        self.assertEqual(
            [b.name for b in builds], ['Android_Build', 'Android_Test']
        )
        build, test = builds
        self.assertEqual(build.kind, Kind.INFRA)
        self.assertEqual(build.problems, ('Process exited with code 1',))
        self.assertTrue(build.log_tail.endswith('Process exited with code 1'))
        self.assertNotIn('cleanup', build.log_tail)

        self.assertEqual(test.kind, Kind.TESTS)
        self.assertEqual(test.problems, ())
        self.assertEqual(test.log_tail, '')
        flaky, new = test.tests
        self.assertEqual(flaky.status, 'FAILURE, 2 of 2 runs')
        self.assertEqual(flaky.known, 'failing since build 5 https://tc/5')
        self.assertEqual(new.known, '')

        # Muted tests don't count; reads can't write.
        paths = [cmd[-1] for cmd, _ in cli.calls]
        self.assertTrue(
            all('muted:false' in p for p in paths if 'testOccurrences' in p)
        )
        self.assertTrue(all(env['TEAMCITY_RO'] == '1' for _, env in cli.calls))

    def test_a_build_without_failed_dependencies_is_read_itself(self):
        cli = FakeCli(
            {
                '/app/rest/builds?locator=': {},
                '/app/rest/builds/id:20': {
                    'id': 20,
                    'buildTypeId': 'Lint',
                    'statusText': 'Tests failed: 1',
                },
                '/app/rest/problemOccurrences': {},
                '/app/rest/testOccurrences': {
                    'testOccurrence': [{'name': 'T', 'status': 'FAILURE'}]
                },
            }
        )
        (build,) = teamcity.TeamCityCi(run=cli).failed_builds(20)
        self.assertEqual(build.name, 'Lint')
        self.assertEqual(build.kind, Kind.TESTS)


class RerunTest(unittest.TestCase):
    def test_restarts_with_writes_allowed(self):
        cli = FakeCli({})
        with mock.patch.dict(os.environ):
            os.environ.pop('TEAMCITY_RO', None)
            self.assertEqual(teamcity.TeamCityCi(run=cli).rerun(10), 'queued')
        self.assertEqual(len(cli.calls), 1)
        cmd, env = cli.calls[0]
        self.assertEqual(cmd, ['teamcity', 'run', 'restart', '10'])
        self.assertNotIn('TEAMCITY_RO', env)

    def test_keeps_a_read_only_environment_read_only(self):
        cli = FakeCli({})
        with mock.patch.dict(os.environ, {'TEAMCITY_RO': '1'}):
            teamcity.TeamCityCi(run=cli).rerun(10)
        self.assertEqual(len(cli.calls), 1)
        _, env = cli.calls[0]
        self.assertEqual(env['TEAMCITY_RO'], '1')


if __name__ == '__main__':
    unittest.main()
