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
    # A full disk, as POSIX and Windows word it.
    re.compile(r'No space left on device'),
    re.compile(r'There is not enough space on the disk'),
    # The agent's workspace volume is still attached to another machine.
    re.compile(r'vol/attach failed'),
    re.compile(r"is in state in-use/attached and can't be attached"),
)

# A `glog` fatal line, as siso writes one before it exits:
# `F1108 13:30:24.969724   11708 glog.go:261] ...`.
_GLOG_FATAL = re.compile(r'\bF\d{4} \d\d:\d\d:\d\d\.\d+ +\d+ \S+:\d+\] .*')

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
    tests: tuple[FailedTest, ...] = ()
    # Build problems other than failed tests, e.g. a step's exit code.
    problems: tuple[str, ...] = ()
    # The end of the failed step's log.
    log_tail: str = ''
    # Whether the CI system stopped it (a timeout, a newer run).
    aborted: bool = False

    @property
    def kind(self) -> Kind:
        """What it most likely needs, from how it failed and its log."""
        if self.aborted or any(
            pattern.search(self.log_tail) for pattern in _INFRA_PATTERNS
        ):
            return Kind.INFRA
        if self.tests and not self.problems:
            return Kind.TESTS
        return Kind.BUILD


@dataclass(frozen=True)
class CheckFailures:
    """A failed GitHub check, and the builds behind it that failed."""

    check: str
    url: str
    system: str
    builds: tuple[FailedBuild, ...] = field(default_factory=tuple)
    # Why the builds couldn't be read, if they couldn't.
    error: str = ''


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


def with_fatal_line(log: str, log_tail: str) -> str:
    """`log_tail`, after `log`'s last `glog` fatal line when the tail doesn't
    have it: siso's output after its fatal error can push it far above the
    end of the log."""
    fatal = _GLOG_FATAL.findall(log)
    if not fatal or fatal[-1] in log_tail:
        return log_tail
    return f'{fatal[-1]}\n[...]\n{log_tail}'
