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

import base64
from collections.abc import Callable
import json
import os
import re
import subprocess
import urllib.parse

from ci import failures
from ci.failures import FailedBuild, FailedTest

# A build's page: `.../buildConfiguration/<job>/<id>`, or the older
# `viewLog.html?buildId=<id>`.
_BUILD_ID = re.compile(r'/buildConfiguration/[^/]+/(\d+)|[?&]buildId=(\d+)')

# How many lines of a failed test's output a report keeps; the assertion is
# at the end.
_TEST_DETAILS_LINES = 40

# How many of a build's failed tests get their history read, one request
# each, and how many of the branch's latest runs it covers.
_HISTORY_TESTS = 10
_HISTORY_RUNS = 200
# How many failing revisions a configuration's history names.
_HISTORY_REVISIONS = 5


def _brave_core_revision(build: dict) -> str:
    """The brave-core commit `build` ran on, shortened. Builds also check
    out the CI's own repository."""
    revisions = (build.get('revisions') or {}).get('revision') or []
    for revision in revisions:
        root = (revision.get('vcs-root-instance') or {}).get('name', '')
        if root.endswith('/brave-core'):
            return revision.get('version', '')[:10]
    return revisions[-1].get('version', '')[:10] if revisions else ''


_EXIT_CODE = re.compile(r'Process exited with code (\d+)')


def _failed_step_tail(log: str) -> str:
    """The lines of a build log leading up to the first step that exited
    non-zero, or the log's last lines when none did.

    A build's last steps are usually cleanup, so its last lines rarely say
    why it failed.
    """
    log_lines = log.splitlines()
    end = len(log_lines)
    for index, line in enumerate(log_lines):
        match = _EXIT_CODE.search(line)
        if match and match[1] != '0':
            end = index + 1
            break
    return '\n'.join(log_lines[max(0, end - failures.LOG_TAIL_LINES) : end])


def _build_id(url: str) -> int:
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

    def failed_builds(self, url: str) -> list[FailedBuild]:
        """The builds that failed in the chain of the aggregate at `url`, or
        the aggregate itself when it has no failed dependencies.

        A build that failed only because one it depends on did is left out:
        the one it depends on says why.
        """
        aggregate = _build_id(url)
        fields = 'id,buildTypeId,branchName,statusText,webUrl'
        builds = self._api(
            '/app/rest/builds?locator=snapshotDependency:'
            f'(to:(id:{aggregate}),includeInitial:false),status:FAILURE'
            f'&fields=build({fields})'
        ).get('build', [])
        if not builds:
            builds = [
                self._api(f'/app/rest/builds/id:{aggregate}?fields={fields}')
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
        tests = self._failed_tests(teamcity_id, build.get('branchName', ''))
        log_tail = ''
        if step_problems:
            log = self._log(teamcity_id)
            log_tail = failures.with_fatal_line(log, _failed_step_tail(log))
        return FailedBuild(
            name=build.get('buildTypeId', str(teamcity_id)),
            url=build.get('webUrl', ''),
            summary=build.get('statusText', ''),
            tests=tests,
            problems=step_problems,
            log_tail=log_tail,
        )

    def _failed_tests(
        self, teamcity_id: int, branch: str
    ) -> tuple[FailedTest, ...]:
        """The build's failed tests, leaving out muted ones, which TeamCity
        doesn't count against the build."""
        fields = (
            'testOccurrence(name,status,details,newFailure,test(id),'
            'firstFailed(build(id,webUrl)),invocations(count,failed))'
        )
        occurrences = self._api(
            f'/app/rest/testOccurrences?locator=build:(id:{teamcity_id}),'
            f'status:FAILURE,muted:false,count:200&fields={fields}'
        ).get('testOccurrence', [])
        tests = []
        for index, test in enumerate(occurrences):
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
            test_id = (test.get('test') or {}).get('id')
            history = ()
            if branch and test_id and index < _HISTORY_TESTS:
                history = self._history(test_id, branch)
            tests.append(
                FailedTest(
                    name=test.get('name', ''),
                    status=status,
                    details=failures.tail(
                        test.get('details', ''), _TEST_DETAILS_LINES
                    ),
                    known=known,
                    mocha=failures.mocha_failures(test.get('details', '')),
                    history=history,
                )
            )
        return tuple(tests)

    def _history(self, test_id: str, branch: str) -> tuple[str, ...]:
        """The test's results in `branch`'s latest runs, a line per build
        configuration: how many failed, and on which revisions."""
        name = urllib.parse.quote(base64.b64encode(branch.encode()).decode())
        occurrences = self._api(
            f'/app/rest/testOccurrences?locator=test:(id:{test_id}),'
            f'branch:(name:($base64:{name})),count:{_HISTORY_RUNS}'
            '&fields=testOccurrence(status,build(buildTypeId,'
            'revisions(revision(version,vcs-root-instance(name)))))'
        ).get('testOccurrence', [])
        # Per configuration, per revision: [failed, runs].
        results: dict[str, dict[str, list[int]]] = {}
        for occurrence in occurrences:
            build = occurrence.get('build') or {}
            revisions = results.setdefault(build.get('buildTypeId', ''), {})
            counts = revisions.setdefault(_brave_core_revision(build), [0, 0])
            counts[0] += occurrence.get('status') == 'FAILURE'
            counts[1] += 1
        lines = []
        for configuration, revisions in results.items():
            failed = sum(counts[0] for counts in revisions.values())
            runs = sum(counts[1] for counts in revisions.values())
            line = f'`{configuration}` on `{branch}`: failed {failed} of {runs}'
            failing = [
                f'`{revision}` ({counts[0]} of {counts[1]})'
                for revision, counts in revisions.items()
                if counts[0]
            ]
            if failing:
                line += ', on ' + ', '.join(failing[:_HISTORY_REVISIONS])
                if len(failing) > _HISTORY_REVISIONS:
                    line += ', ...'
            lines.append(line)
        return tuple(lines)

    def full_log(self, url: str) -> str:
        """The whole build log of the build at `url`."""
        return self._log(_build_id(url))

    def _log(self, build: int) -> str:
        return self._cli(
            'api', '--raw', f'/downloadBuildLog.html?buildId={build}&plain=true'
        )

    def rerun(self, url: str) -> str:
        """Queues the build at `url` again, with the same job, branch,
        revision and parameters, and returns the CLI's output.

        For an aggregate, TeamCity reuses the dependencies that succeeded,
        and reruns those that failed.
        """
        return self._cli('run', 'restart', str(_build_id(url)), write=True)
