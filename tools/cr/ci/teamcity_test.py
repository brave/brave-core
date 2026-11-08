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
from ci import failures, teamcity  # noqa: E402  -- after _boot
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


def _url(build: int) -> str:
    return f'https://teamcity.brave.com:443/buildConfiguration/X/{build}'


class BuildIdTest(unittest.TestCase):
    def test_build_configuration_url(self):
        self.assertEqual(
            teamcity._build_id(
                'https://teamcity.brave.com:443/buildConfiguration/'
                'BraveBrowser_Android_Aggregate/753605'
            ),
            753605,
        )

    def test_view_log_url(self):
        self.assertEqual(
            teamcity._build_id('https://tc/viewLog.html?buildId=12&tab=log'),
            12,
        )

    def test_rejects_other_urls(self):
        with self.assertRaises(ValueError):
            teamcity._build_id('https://ci.brave.com/job/x/1/')


class FailedStepTailTest(unittest.TestCase):
    def test_stops_at_the_first_failed_step(self):
        log = '\n'.join(
            [f'[Step 1/3] spew {i}' for i in range(100)]
            + [
                '[Step 2/3] error: it broke',
                '[Step 2/3] Process exited with code 1',
                '[Step 3/3] cleanup',
                '[Step 3/3] Process exited with code 0',
            ]
        )
        tail = teamcity._failed_step_tail(log).splitlines()
        self.assertEqual(len(tail), failures.LOG_TAIL_LINES)
        self.assertEqual(tail[-1], '[Step 2/3] Process exited with code 1')

    def test_without_a_failed_step_takes_the_end(self):
        log = '\n'.join(f'line {i}' for i in range(100))
        tail = teamcity._failed_step_tail(log).splitlines()
        self.assertEqual(len(tail), failures.LOG_TAIL_LINES)
        self.assertEqual(tail[-1], 'line 99')


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
        builds = teamcity.TeamCityCi(run=cli).failed_builds(_url(10))

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

    def test_a_fatal_line_above_the_failed_step_leads_the_tail(self):
        fatal = (
            'F1108 13:30:24.969724   11708 glog.go:261] log: exiting: '
            'There is not enough space on the disk.'
        )
        log = '\n'.join(
            [f'[Step 1/2] {fatal}']
            + ['[Step 1/2] I1108 spew'] * 100
            + ['[Step 1/2] Process exited with code 1']
        )
        cli = FakeCli(
            {
                '/app/rest/builds?locator=': {},
                '/app/rest/builds/id:30': {'id': 30, 'buildTypeId': 'Build'},
                '/app/rest/problemOccurrences': {
                    'problemOccurrence': [{'type': 'TC_EXIT_CODE'}]
                },
                '/app/rest/testOccurrences': {},
                '/downloadBuildLog.html?buildId=30': log,
            }
        )
        [build] = teamcity.TeamCityCi(run=cli).failed_builds(_url(30))
        self.assertTrue(build.log_tail.startswith(f'{fatal}\n[...]\n'))
        self.assertEqual(build.kind, Kind.INFRA)

    def test_full_log(self):
        cli = FakeCli({'/downloadBuildLog.html?buildId=7&plain=true': _LOG})
        self.assertEqual(teamcity.TeamCityCi(run=cli).full_log(_url(7)), _LOG)
        self.assertEqual(cli.calls[0][1]['TEAMCITY_RO'], '1')

    def test_full_log_drops_the_times_but_keeps_the_severity(self):
        log = (
            "Build 'Brave Browser / android / test'\n"
            '[15:13:02]i: Changed build status to failed\n'
            '[15:24:21]W: Build was removed from the queue\n'
        )
        cli = FakeCli({'/downloadBuildLog.html?buildId=7&plain=true': log})
        ci = teamcity.TeamCityCi(run=cli)
        self.assertEqual(
            ci.full_log(_url(7), timestamps=False),
            "Build 'Brave Browser / android / test'\n"
            'i: Changed build status to failed\n'
            'W: Build was removed from the queue\n',
        )
        self.assertEqual(ci.full_log(_url(7)), log)

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
        (build,) = teamcity.TeamCityCi(run=cli).failed_builds(_url(20))
        self.assertEqual(build.name, 'Lint')
        self.assertEqual(build.kind, Kind.TESTS)

    def test_reads_a_failed_tests_history_on_the_branch(self):
        def run(config, revision, status):
            return {
                'status': status,
                'build': {
                    'buildTypeId': config,
                    'revisions': {
                        'revision': [
                            {
                                'version': 'c1' * 20,
                                'vcs-root-instance': {
                                    'name': 'https://github.com/brave/teamcity'
                                },
                            },
                            {
                                'version': revision * 40,
                                'vcs-root-instance': {
                                    'name': 'https://github.com/brave/brave-core'
                                },
                            },
                        ]
                    },
                },
            }

        cli = FakeCli(
            {
                '/app/rest/builds?locator=': {},
                '/app/rest/builds/id:30': {
                    'id': 30,
                    'buildTypeId': 'Linux_Test',
                    'branchName': 'cr158',
                },
                '/app/rest/problemOccurrences': {},
                '/app/rest/testOccurrences?locator=build:': {
                    'testOccurrence': [
                        {
                            'name': 'brave_browser_tests: Suite.Mocha',
                            'status': 'FAILURE',
                            'test': {'id': '77'},
                            'details': (
                                '[1:1:0:INFO:CONSOLE:58] "   failed: Page t\n'
                                'AssertionError: no", source: x.js (58)'
                            ),
                        }
                    ]
                },
                '/app/rest/testOccurrences?locator=test:(id:77)': {
                    'testOccurrence': [
                        run('Linux_Test', 'a', 'FAILURE'),
                        run('Linux_Test', 'a', 'SUCCESS'),
                        run('Linux_Test', 'b', 'SUCCESS'),
                        run('Mac_Test', 'b', 'SUCCESS'),
                    ]
                },
            }
        )
        (build,) = teamcity.TeamCityCi(run=cli).failed_builds(_url(30))
        (test,) = build.tests
        self.assertEqual(test.mocha, ('Page t\nAssertionError: no',))
        self.assertEqual(
            test.history,
            (
                '`Linux_Test` on `cr158`: failed 1 of 3, on `aaaaaaaaaa` '
                '(1 of 2)',
                '`Mac_Test` on `cr158`: failed 0 of 1',
            ),
        )
        (history_path,) = [
            cmd[-1] for cmd, _ in cli.calls if 'locator=test:' in cmd[-1]
        ]
        self.assertIn('branch:(name:($base64:Y3IxNTg%3D))', history_path)


class RerunTest(unittest.TestCase):
    def test_restarts_with_writes_allowed(self):
        cli = FakeCli({})
        with mock.patch.dict(os.environ):
            os.environ.pop('TEAMCITY_RO', None)
            self.assertEqual(
                teamcity.TeamCityCi(run=cli).rerun(_url(10)), 'queued'
            )
        self.assertEqual(len(cli.calls), 1)
        cmd, env = cli.calls[0]
        self.assertEqual(cmd, ['teamcity', 'run', 'restart', '10'])
        self.assertNotIn('TEAMCITY_RO', env)

    def test_keeps_a_read_only_environment_read_only(self):
        cli = FakeCli({})
        with mock.patch.dict(os.environ, {'TEAMCITY_RO': '1'}):
            teamcity.TeamCityCi(run=cli).rerun(_url(10))
        self.assertEqual(len(cli.calls), 1)
        _, env = cli.calls[0]
        self.assertEqual(env['TEAMCITY_RO'], '1')


if __name__ == '__main__':
    unittest.main()
