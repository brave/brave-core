# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""What failed on a pull request, across the CI systems that ran its checks."""

from __future__ import annotations

from dataclasses import dataclass

from ci import github, systems
from ci.failures import CheckFailures, FailedBuild, Kind


@dataclass(frozen=True)
class Report:
    checks: github.PullRequestChecks
    # The failed checks, with the builds behind each.
    failed: tuple[CheckFailures, ...]

    @property
    def pending(self) -> list[github.Check]:
        return self.checks.in_state(github.State.PENDING)

    def counts(self) -> str:
        return (
            f'{len(self.failed)} failed, {len(self.pending)} pending of '
            f'{len(self.checks.checks)} checks.'
        )

    def summary(self) -> list[str]:
        """A line per failed check: its builds and their kinds, or why they
        couldn't be read."""
        lines = []
        for check in self.failed:
            kinds = ', '.join(f'{b.name}: {b.kind}' for b in check.builds)
            lines.append(
                f'{check.check}: {kinds or check.error or "no failed builds"}'
            )
        return lines

    def markdown(self) -> str:
        """The whole report, a section per failed check."""
        checks = self.checks
        return (
            f'# CI failures of {checks.branch} (PR {checks.number}) at '
            f'{checks.head}\n\n{self.counts()}\n\n' + _sections(self.failed)
        )


def of(pr: int | str) -> Report:
    """What failed on pull request `pr`, a number or a branch."""
    checks = github.pr_checks(pr)
    return Report(checks, tuple(_collect(checks)))


def _collect(checks: github.PullRequestChecks) -> list[CheckFailures]:
    """The failed builds behind each failed check. A check whose builds
    can't be read (no credentials, a CI system this package doesn't read)
    is still listed, with why."""
    result = []
    for check in checks.in_state(github.State.FAILURE):
        builds: list[FailedBuild] = []
        error = ''
        try:
            builds = systems.failed_builds(check.url)
        except systems.Unsupported:
            error = f'{check.system or "unknown"} checks are not read'
        except Exception as e:  # pylint: disable=broad-exception-caught
            # One unreadable check shouldn't hide the others.
            error = f'{type(e).__name__}: {e}'
        result.append(
            CheckFailures(
                check=check.name,
                url=check.url,
                system=check.system,
                builds=tuple(builds),
                error=error,
            )
        )
    return result


def _fence(text: str) -> str:
    fence = '````' if '```' in text else '```'
    return f'{fence}\n{text}\n{fence}'


def _sections(failed: tuple[CheckFailures, ...]) -> str:
    if not failed:
        return 'No failed checks.\n'
    sections = []
    for check in failed:
        lines = [f'## {check.check} ({check.system})', '', check.url, '']
        if check.error:
            lines += [f'Could not read its builds: {check.error}', '']
        for build in check.builds:
            lines += [
                f'### {build.name}: {build.kind}',
                '',
                f'{build.summary} ({build.url})',
                '',
            ]
            lines += [f'- Problem: {problem}' for problem in build.problems]
            for test in build.tests:
                known = f' ({test.known})' if test.known else ''
                lines.append(f'- Test `{test.name}`: {test.status}{known}')
                lines += [
                    f'  - Mocha test failed: `{failure.splitlines()[0]}`'
                    for failure in test.mocha
                ]
                lines += [f'  - History: {line}' for line in test.history]
            if build.problems or build.tests:
                lines.append('')
            for test in build.tests:
                if test.mocha:
                    lines += [
                        f'`{test.name}`, Mocha:',
                        '',
                        _fence('\n\n'.join(test.mocha)),
                        '',
                    ]
                if test.details:
                    lines += [f'`{test.name}`:', '', _fence(test.details), '']
            if build.log_tail and build.kind != Kind.TESTS:
                lines += ['Log:', '', _fence(build.log_tail), '']
        sections.append('\n'.join(lines))
    return '\n'.join(sections)
