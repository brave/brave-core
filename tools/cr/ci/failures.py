# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""What failed on CI, in the same shape whichever CI system ran it."""

from __future__ import annotations

from dataclasses import dataclass, field
import enum
import re


class Kind(enum.StrEnum):
    """What a failed build most likely needs. A hint for whoever reads the
    report, not a verdict: the log tail is there to check it against."""

    # The CI machine or its workspace, not the code: a rerun, maybe a clean
    # one, fixes it.
    INFRA = 'infra'
    # Only tests failed.
    TESTS = 'tests'
    # A build or check step failed.
    BUILD = 'build'


# Log lines that put a failure on the CI machine rather than the code.
_INFRA_PATTERNS = (
    # A workspace built with ninja, now built with siso.
    re.compile(r'Run gn clean before switching from ninja to siso'),
    re.compile(r'No space left on device'),
)

# How many lines of a failed step's log a report keeps.
LOG_TAIL_LINES = 60


@dataclass(frozen=True)
class FailedTest:
    name: str
    # As the CI system reports it, e.g. FAILURE, CRASH, REGRESSION.
    status: str
    # The test's output, trimmed.
    details: str = ''
    # Evidence that the failure predates this build (failing for several
    # builds, already failing on an earlier one), or empty.
    known: str = ''
    # For a WebUI Mocha browser test, each failed Mocha test: its title on
    # the first line, then its error and stack.
    mocha: tuple[str, ...] = ()
    # The test's results in the branch's earlier builds, one line per build
    # configuration.
    history: tuple[str, ...] = ()


@dataclass(frozen=True)
class FailedBuild:
    """One failed build of a check: a TeamCity build in an aggregate's chain,
    or a Jenkins pipeline stage."""

    name: str
    url: str
    # The CI system's one-line summary of the failure.
    summary: str
    kind: Kind
    tests: tuple[FailedTest, ...] = ()
    # Build problems other than failed tests, e.g. a step's exit code.
    problems: tuple[str, ...] = ()
    # The end of the failed step's log.
    log_tail: str = ''


@dataclass(frozen=True)
class CheckFailures:
    """A failed GitHub check, and the builds behind it that failed."""

    check: str
    url: str
    system: str
    builds: tuple[FailedBuild, ...] = field(default_factory=tuple)
    # Why the builds couldn't be read, if they couldn't.
    error: str = ''


def classify(
    log_tail: str, tests: tuple[FailedTest, ...], *, step_failed: bool
) -> Kind:
    """The `Kind` of a failed build, from its log and what failed.

    `step_failed` is whether a build step failed, rather than only tests.
    """
    if any(pattern.search(log_tail) for pattern in _INFRA_PATTERNS):
        return Kind.INFRA
    if tests and not step_failed:
        return Kind.TESTS
    return Kind.BUILD


_EXIT_CODE = re.compile(r'Process exited with code (\d+)')


def failed_step_tail(log: str, lines: int = LOG_TAIL_LINES) -> str:
    """The lines of a TeamCity build log leading up to the first step that
    exited non-zero, or the log's last lines when none did.

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
    return '\n'.join(log_lines[max(0, end - lines) : end])


# `mocha_adapter_simple.ts` logs a failure as one console message,
# "<indent> failed: <title>\n<stack>", which Chromium logs as
# `...:CONSOLE:58] "<message>", source: <url> (58)`.
_MOCHA_FAILED = re.compile(r':CONSOLE[^\]]*\] "\s*failed: (.*)')
_CONSOLE_END = re.compile(r'", source: \S+ \(\d+\)$')

# How many lines of a Mocha failure a report keeps: the title, the error
# and the first frames.
_MOCHA_LINES = 8


def mocha_failures(output: str) -> tuple[str, ...]:
    """The Mocha failures in a WebUI browser test's output, each as its
    title, then its error and stack. A retry logs the same failure again;
    it is kept once."""
    found: list[str] = []
    lines = output.splitlines()
    for index, line in enumerate(lines):
        match = _MOCHA_FAILED.search(line)
        if not match:
            continue
        block = []
        for message_line in [match[1], *lines[index + 1 :]]:
            end = _CONSOLE_END.search(message_line)
            block.append(message_line[: end.start()] if end else message_line)
            if end or len(block) == _MOCHA_LINES:
                break
        failure = '\n'.join(block)
        if failure not in found:
            found.append(failure)
    return tuple(found)


def tail(text: str, lines: int = LOG_TAIL_LINES) -> str:
    return '\n'.join(text.splitlines()[-lines:])


def _fence(text: str) -> str:
    fence = '````' if '```' in text else '```'
    return f'{fence}\n{text}\n{fence}'


def to_markdown(failures: list[CheckFailures]) -> str:
    """A report of `failures`, one section per check."""
    if not failures:
        return 'No failed checks.\n'
    sections = []
    for check in failures:
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
