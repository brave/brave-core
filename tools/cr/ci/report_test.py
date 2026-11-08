#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from unittest import mock

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import github, report, systems  # noqa: E402  -- after _boot
from ci.failures import (  # noqa: E402  -- after _boot
    CheckFailures,
    FailedBuild,
    FailedTest,
)
from ci.github import Check, PullRequestChecks, State  # noqa: E402

_TEAMCITY = 'https://teamcity.brave.com:443/buildConfiguration/X_Aggregate/7'
_JENKINS = (
    'https://ci.brave.com/job/brave-core-build-pr-windows-x64/job/PR-1/2/'
)
_BUILD = FailedBuild('X_Build', 'https://tc/8', 'Exit code 1')
_CHECKS = PullRequestChecks(
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


class OfTest(unittest.TestCase):
    def test_reads_each_failed_check_from_its_system(self):
        def failed_builds(url):
            if url == _JENKINS:
                raise RuntimeError('no access')
            if url == _TEAMCITY:
                return [_BUILD]
            raise systems.Unsupported(url)

        with (
            mock.patch.object(github, 'pr_checks', return_value=_CHECKS),
            mock.patch.object(
                systems, 'failed_builds', side_effect=failed_builds
            ),
        ):
            found = report.of('cr159')

        failed = found.failed
        self.assertEqual([c.check for c in failed], ['tc', 'jenkins', 'socket'])
        self.assertEqual(failed[0].builds, (_BUILD,))
        self.assertEqual(failed[1].error, 'RuntimeError: no access')
        self.assertEqual(failed[2].error, 'socket.dev checks are not read')
        self.assertEqual(found.counts(), '3 failed, 1 pending of 5 checks.')
        self.assertEqual(
            found.summary(),
            [
                'tc: X_Build: build',
                'jenkins: RuntimeError: no access',
                'socket: socket.dev checks are not read',
            ],
        )
        self.assertEqual([c.name for c in found.pending], ['running'])


def _markdown(*failed: CheckFailures) -> str:
    return report.Report(_CHECKS, failed).markdown()


class MarkdownTest(unittest.TestCase):
    def test_no_failures(self):
        self.assertEqual(
            _markdown(),
            f'# CI failures of cr159 (PR 1) at {"a" * 40}\n\n'
            '0 failed, 1 pending of 5 checks.\n\nNo failed checks.\n',
        )

    def test_lists_builds_tests_and_logs(self):
        text = _markdown(
            CheckFailures(
                check='PR browser (android)',
                url='https://tc/agg',
                system='teamcity',
                builds=(
                    FailedBuild(
                        name='Android_Build',
                        url='https://tc/build',
                        summary='Exit code 1',
                        problems=('Process exited with code 1',),
                        log_tail='error: ```boom```',
                    ),
                    FailedBuild(
                        name='Android_Test',
                        url='https://tc/test',
                        summary='Tests failed: 1',
                        tests=(
                            FailedTest(
                                'Suite.Test',
                                'FAILURE',
                                details='Expected 1',
                                known='failing since build 1',
                            ),
                        ),
                        log_tail='not shown for tests',
                    ),
                ),
            ),
            CheckFailures(
                check='other', url='u', system='x', error='no access'
            ),
        )
        self.assertIn('### Android_Build: build', text)
        self.assertIn('- Problem: Process exited with code 1', text)
        # A log quoting a fence gets a longer one.
        self.assertIn('````\nerror: ```boom```\n````', text)
        self.assertIn(
            '- Test `Suite.Test`: FAILURE (failing since build 1)', text
        )
        self.assertIn('```\nExpected 1\n```', text)
        self.assertNotIn('not shown for tests', text)
        self.assertIn('Could not read its builds: no access', text)

    def test_lists_mocha_failures_and_history(self):
        text = _markdown(
            CheckFailures(
                check='PR browser linux (x64)',
                url='https://tc/agg',
                system='teamcity',
                builds=(
                    FailedBuild(
                        name='Linux_Test',
                        url='https://tc/test',
                        summary='Tests failed: 1',
                        tests=(
                            FailedTest(
                                'Suite.Mocha',
                                'FAILURE',
                                mocha=('Page saves\nAssertionError: no',),
                                history=('`Linux_Test`: failed 1 of 2',),
                            ),
                        ),
                    ),
                ),
            )
        )
        self.assertIn(
            '- Test `Suite.Mocha`: FAILURE\n'
            '  - Mocha test failed: `Page saves`\n'
            '  - History: `Linux_Test`: failed 1 of 2\n',
            text,
        )
        self.assertIn(
            '`Suite.Mocha`, Mocha:\n\n```\nPage saves\nAssertionError: no\n```',
            text,
        )


if __name__ == '__main__':
    unittest.main()
