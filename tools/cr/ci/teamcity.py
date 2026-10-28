# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Reads failed TeamCity builds, and reruns them, through the TeamCity CLI.

The CLI (`teamcity`, https://github.com/JetBrains/teamcity-cli) holds the
credentials: `teamcity auth login` on a workstation, `TEAMCITY_URL` and
`TEAMCITY_TOKEN` on CI. Reads run with `TEAMCITY_RO=1`, so only `rerun` can
change anything on the server.
"""

from __future__ import annotations

from collections.abc import Callable
import json
import os
import re
import subprocess

from ci import failures
from ci.failures import FailedBuild, FailedTest

# A build's page: `.../buildConfiguration/<job>/<id>`, or the older
# `viewLog.html?buildId=<id>`.
_BUILD_ID = re.compile(r'/buildConfiguration/[^/]+/(\d+)|[?&]buildId=(\d+)')

# How many lines of a failed test's output a report keeps; the assertion is
# at the end.
_TEST_DETAILS_LINES = 40


def build_id(url: str) -> int:
    """The id of the build at `url`."""
    match = _BUILD_ID.search(url)
    if not match:
        raise ValueError(f'Not a TeamCity build URL: {url}')
    return int(match[1] or match[2])


class TeamCityCi:
    def __init__(
        self,
        run: Callable[..., subprocess.CompletedProcess] = subprocess.run,
    ) -> None:
        self._run = run

    def _cli(self, *args: str, write: bool = False) -> str:
        env = {**os.environ, 'NO_COLOR': '1', 'TEAMCITY_NO_UPDATE': '1'}
        if not write:
            env['TEAMCITY_RO'] = '1'
        return self._run(
            ['teamcity', *args],
            check=True,
            capture_output=True,
            text=True,
            encoding='utf-8',
            env=env,
        ).stdout

    def _api(self, path: str) -> dict:
        return json.loads(self._cli('api', '--raw', path))

    def failed_builds(self, aggregate: int) -> list[FailedBuild]:
        """The builds that failed in `aggregate`'s chain, or `aggregate`
        itself when it has no failed dependencies.

        A build that failed only because one it depends on did is left out:
        the one it depends on says why.
        """
        fields = 'build(id,buildTypeId,statusText,webUrl)'
        builds = self._api(
            '/app/rest/builds?locator=snapshotDependency:'
            f'(to:(id:{aggregate}),includeInitial:false),status:FAILURE'
            f'&fields={fields}'
        ).get('build', [])
        if not builds:
            builds = [
                self._api(
                    f'/app/rest/builds/id:{aggregate}'
                    '?fields=id,buildTypeId,statusText,webUrl'
                )
            ]
        found = (self._failed_build(build) for build in builds)
        return [build for build in found if build is not None]

    def _failed_build(self, build: dict) -> FailedBuild | None:
        teamcity_id = build['id']
        problems = self._api(
            f'/app/rest/problemOccurrences?locator=build:(id:{teamcity_id}),'
            'count:100&fields=problemOccurrence(type,details)'
        ).get('problemOccurrence', [])
        types = [problem.get('type', '') for problem in problems]
        if types and all('SNAPSHOT_DEPENDENCY' in kind for kind in types):
            return None
        # Failed tests are a problem of their own, listed as tests instead.
        step_problems = tuple(
            (problem.get('details') or problem.get('type', '')).strip()
            for problem in problems
            if not problem.get('type', '').startswith('TC_FAILED_TESTS')
        )
        tests = self._failed_tests(teamcity_id)
        log_tail = ''
        if step_problems:
            log_tail = failures.failed_step_tail(
                self._cli(
                    'api',
                    '--raw',
                    f'/downloadBuildLog.html?buildId={teamcity_id}&plain=true',
                )
            )
        return FailedBuild(
            name=build.get('buildTypeId', str(teamcity_id)),
            url=build.get('webUrl', ''),
            summary=build.get('statusText', ''),
            kind=failures.classify(
                log_tail, tests, step_failed=bool(step_problems)
            ),
            tests=tests,
            problems=step_problems,
            log_tail=log_tail,
        )

    def _failed_tests(self, teamcity_id: int) -> tuple[FailedTest, ...]:
        """The build's failed tests, leaving out muted ones, which TeamCity
        doesn't count against the build."""
        fields = (
            'testOccurrence(name,status,details,newFailure,'
            'firstFailed(build(id,webUrl)),invocations(count,failed))'
        )
        occurrences = self._api(
            f'/app/rest/testOccurrences?locator=build:(id:{teamcity_id}),'
            f'status:FAILURE,muted:false,count:200&fields={fields}'
        ).get('testOccurrence', [])
        tests = []
        for test in occurrences:
            status = test.get('status', '')
            invocations = test.get('invocations') or {}
            if invocations.get('count', 0) > 1:
                status += (
                    f', {invocations.get("failed", 0)} of '
                    f'{invocations["count"]} runs'
                )
            known = ''
            first = (test.get('firstFailed') or {}).get('build') or {}
            if not test.get('newFailure') and first:
                known = (
                    f'failing since build {first.get("id")} '
                    f'{first.get("webUrl", "")}'
                ).strip()
            tests.append(
                FailedTest(
                    name=test.get('name', ''),
                    status=status,
                    details=failures.tail(
                        test.get('details', ''), _TEST_DETAILS_LINES
                    ),
                    known=known,
                )
            )
        return tuple(tests)

    def rerun(self, build: int) -> str:
        """Queues `build` again, with the same job, branch, revision and
        parameters, and returns the CLI's output.

        For an aggregate, TeamCity reuses the dependencies that succeeded,
        and reruns those that failed.
        """
        return self._cli('run', 'restart', str(build), write=True)
