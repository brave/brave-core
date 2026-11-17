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

# How many of a build's failed tests get their history read, one request
# each, and how many of the branch's latest runs it covers.
_HISTORY_TESTS = 10
_HISTORY_RUNS = 200
# How many failing revisions a configuration's history names.
_HISTORY_REVISIONS = 5


def _branch_locator(branch: str) -> str:
    """A `branch:` locator dimension matching `branch` exactly, whatever
    characters it has."""
    name = urllib.parse.quote(base64.b64encode(branch.encode()).decode())
    return f'branch:(name:($base64:{name}))'


def _dependency(build: dict) -> str:
    """A line naming a snapshot dependency and its result."""
    result = build.get('status', '')
    if build.get('state', 'finished') != 'finished':
        result = build['state']
    return (
        f'`{build.get("buildTypeId", "")}` {build.get("id")}: {result} '
        f'({build.get("webUrl", "")})'
    )


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

# The time before each log line. The severity after it (`i:`, `W:`, `E:`) is
# kept.
_TIMESTAMP = re.compile(r'^\[\d\d:\d\d:\d\d\]', re.MULTILINE)

# Colours a build's output keeps in its log messages, where TeamCity turns
# their escape character into a space.
_ANSI_ESCAPE = re.compile(r'[\x1b ]?\[\d+(?:;\d+)*m')

# The most messages the build log viewer's endpoint returns at once.
_MESSAGES_PER_PAGE = 100

# How long a CLI call may take. A whole build log is rendered as it is sent,
# and a build with thousands of tests logs tens of megabytes, which can take
# much longer than any other call.
_CLI_TIMEOUT = 120
_FULL_LOG_TIMEOUT = 900


class TeamCityError(RuntimeError):
    """A CLI call that failed or timed out, with what the CLI said."""


def _message_text(message: dict) -> str:
    return _ANSI_ESCAPE.sub('', message.get('text') or '')


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

    def _cli(
        self, *args: str, write: bool = False, timeout: int = _CLI_TIMEOUT
    ) -> str:
        env = {**os.environ, 'NO_COLOR': '1', 'TEAMCITY_NO_UPDATE': '1'}
        if not write:
            env['TEAMCITY_RO'] = '1'
        try:
            return self._run(
                ['teamcity', *args],
                check=True,
                capture_output=True,
                text=True,
                encoding='utf-8',
                env=env,
                timeout=timeout,
            ).stdout
        except subprocess.TimeoutExpired as e:
            raise TeamCityError(
                f'`teamcity {" ".join(args)}` took over {timeout}s'
            ) from e
        except subprocess.CalledProcessError as e:
            said = failures.tail((e.stderr or e.stdout or '').strip(), 5)
            raise TeamCityError(
                f'`teamcity {" ".join(args)}` exited with {e.returncode}'
                + (f': {said}' if said else '')
            ) from e

    def _api(self, path: str) -> dict:
        return json.loads(self._cli('api', '--raw', path))

    def _messages(self, build: int, query: str) -> list[dict]:
        """A page of build `build`'s log messages, every block expanded, as
        the build log viewer reads them: up to `_MESSAGES_PER_PAGE` from the
        `messageId` given, or the build's errors with `filter=err`."""
        return self._api(
            f'/app/messages?buildId={build}&expandAll=true&{query}'
        ).get('messages', [])

    def _failed_step_log(self, build: int) -> str:
        """The last lines of the first step that exited non-zero, read without
        the whole log, which a build with thousands of tests makes too big to
        download. Empty when no step did.

        A message's id is its position in the log, so the page that ends at
        the step's exit line is the end of the step. The build's other
        errors, a `glog` fatal line above the tail among them, are logged as
        errors too.
        """
        errors = self._messages(build, 'filter=err')
        exit_line = next(
            (
                message
                for message in errors
                if (match := _EXIT_CODE.search(_message_text(message)))
                and match[1] != '0'
            ),
            None,
        )
        if exit_line is None:
            return ''
        step = next(
            (
                message['id']
                for message in errors
                if message.get('blockType') == 'teamcity-build-step-type'
                and message['id'] < exit_line['id']
            ),
            0,
        )
        start = max(step, exit_line['id'] - _MESSAGES_PER_PAGE + 1)
        page = self._messages(
            build,
            f'filter=all&messageId={start}&count={_MESSAGES_PER_PAGE}',
        )
        lines = [
            _message_text(message)
            for message in page
            if start <= message['id'] <= exit_line['id']
        ]
        return failures.with_fatal_line(
            '\n'.join(map(_message_text, errors)),
            '\n'.join(lines[-failures.LOG_TAIL_LINES :]),
        )

    def _failed_test_occurrences(self, build: int, fields: str) -> list[dict]:
        return self._api(
            f'/app/rest/testOccurrences?locator=build:(id:{build}),'
            f'status:FAILURE,muted:false,count:200&fields={fields}'
        ).get('testOccurrence', [])

    def failed_step_log(self, url: str) -> str:
        """The end of the failed step of the build at `url`, as a report has
        it: the part of the log that says why it failed. When no step failed
        but tests did, each failed test's whole output instead."""
        build = _build_id(url)
        return self._failed_step_log(build) or '\n\n'.join(
            f'{test.get("name", "")}:\n{test.get("details", "")}'.rstrip()
            for test in self._failed_test_occurrences(
                build, 'testOccurrence(name,details)'
            )
        )

    def failed_builds(self, url: str) -> list[FailedBuild]:
        """The builds that failed in the chain of the aggregate at `url`, or
        the aggregate itself when it has no failed dependencies.

        A build that failed only because one it depends on did is left out:
        the one it depends on says why.
        """
        aggregate = _build_id(url)
        fields = (
            'id,buildTypeId,branchName,statusText,webUrl,'
            'snapshot-dependencies(build(id,buildTypeId,status,state,webUrl))'
        )
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
        branch = build.get('branchName', '')
        tests = self._failed_tests(teamcity_id, branch)
        log_tail = ''
        if step_problems:
            try:
                log_tail = self._failed_step_log(teamcity_id)
            except (TeamCityError, ValueError, KeyError) as e:
                # The rest of what failed still says much of why.
                log_tail = f"(The failed step's log could not be read: {e})"
        dependencies = (build.get('snapshot-dependencies') or {}).get(
            'build'
        ) or []
        previous = ''
        if branch and build.get('buildTypeId'):
            previous = self._previous(teamcity_id, build['buildTypeId'], branch)
        return FailedBuild(
            name=build.get('buildTypeId', str(teamcity_id)),
            url=build.get('webUrl', ''),
            summary=build.get('statusText', ''),
            tests=tests,
            problems=step_problems,
            log_tail=log_tail,
            dependencies=tuple(_dependency(d) for d in dependencies),
            previous=previous,
        )

    def _previous(self, teamcity_id: int, job: str, branch: str) -> str:
        """The result of `job`'s last finished build on `branch` before
        build `teamcity_id`, and the brave-core commit it built."""
        # `untilBuild` includes the build itself.
        builds = self._api(
            f'/app/rest/builds?locator=buildType:(id:{job}),'
            f'{_branch_locator(branch)},state:finished,'
            f'untilBuild:(id:{teamcity_id}),count:2'
            '&fields=build(id,status,statusText,webUrl,'
            'revisions(revision(version,vcs-root-instance(name))))'
        ).get('build', [])
        build = next((b for b in builds if b.get('id') != teamcity_id), None)
        if build is None:
            return ''
        line = (
            f'`{job}` {build.get("id")} on `{branch}`: '
            f'{build.get("status", "")}'
        )
        if build.get('statusText'):
            line += f', {build["statusText"]}'
        revision = _brave_core_revision(build)
        if revision:
            line += f', at `{revision}`'
        return f'{line} ({build.get("webUrl", "")})'

    def _failed_tests(
        self, teamcity_id: int, branch: str
    ) -> tuple[FailedTest, ...]:
        """The build's failed tests, leaving out muted ones, which TeamCity
        doesn't count against the build."""
        fields = (
            'testOccurrence(name,status,details,newFailure,test(id),'
            'firstFailed(build(id,webUrl)),invocations(count,failed))'
        )
        occurrences = self._failed_test_occurrences(teamcity_id, fields)
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
                    details=failures.test_output(test.get('details', '')),
                    known=known,
                    mocha=failures.mocha_failures(test.get('details', '')),
                    history=history,
                )
            )
        return tuple(tests)

    def _history(self, test_id: str, branch: str) -> tuple[str, ...]:
        """The test's results in `branch`'s latest runs, a line per build
        configuration: how many failed, and on which revisions."""
        occurrences = self._api(
            f'/app/rest/testOccurrences?locator=test:(id:{test_id}),'
            f'{_branch_locator(branch)},count:{_HISTORY_RUNS}'
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

    def full_log(self, url: str, *, timestamps: bool = True) -> str:
        """The whole build log of the build at `url`, without its lines'
        timestamps if asked. Slow for a big build: `failed_step_log` reads
        its end."""
        log = self._cli(
            'api',
            '--raw',
            f'/downloadBuildLog.html?buildId={_build_id(url)}&plain=true',
            timeout=_FULL_LOG_TIMEOUT,
        )
        return log if timestamps else _TIMESTAMP.sub('', log)

    def rerun(self, url: str) -> str:
        """Queues the build at `url` again, with the same job, branch,
        revision and parameters, and returns the CLI's output.

        For an aggregate, TeamCity reuses the dependencies that succeeded,
        and reruns those that failed.
        """
        return self._cli('run', 'restart', str(_build_id(url)), write=True)
