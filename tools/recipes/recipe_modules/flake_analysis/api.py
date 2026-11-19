# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `flake_analysis` module API."""

from __future__ import annotations

import dataclasses
import enum
import re
from collections.abc import Mapping
from dataclasses import dataclass
from typing import TYPE_CHECKING

from PB.recipe_modules.brave.flake_analysis.properties import InputProperties
from recipe_api import RecipeApi
from test_results import TestFailure, read_summary

if TYPE_CHECKING:
    from recipe_modules import flake_analysis
    from recipe_modules.brave_tests import TestRun
    from recipe_modules.isolate import Isolate

# How often a failing test is rerun: enough for a flaky one to both pass and
# fail.
REPEATS = 10

# Past this many failing tests in a suite, none are rerun, as repeating them
# all would take too long.
MAX_RERUN_TESTS = 100

# The upstream builders comparable to a Brave build on each host.
UPSTREAM_BUILDERS = {
    'linux': r'^(linux-rel|Linux Tests)$',
}

# `is_upstream_flake.py`'s exit codes when it answers.
_UPSTREAM_ANSWERED = (0, 3)

_FILTER_FILE_FLAG = '--test-launcher-filter-file='


class Local(enum.Enum):
    """How a failing test did here."""

    # Failed every rerun.
    DETERMINISTIC = 'deterministic'
    # Passed on a retry, or both passed and failed when rerun.
    FLAKY = 'flaky'
    # Passed every rerun.
    NOT_REPRODUCED = 'not_reproduced'
    # Not rerun: no isolate, too many failures, or no results.
    NOT_RERUN = 'not_rerun'


# Upstream's statuses, as `is_upstream_flake.py` reports them, that exonerate
# a test.
_UPSTREAM_CONFIRMED = frozenset({'flaky', 'failing'})


@dataclass(frozen=True)
class Rerunnable:
    """A suite's isolate, and the arguments its spec runs it with."""

    isolate: Isolate
    args: tuple[str, ...] = ()


@dataclass(frozen=True)
class Verdict:
    """What a failing test did here and upstream."""

    suite: str
    test: str
    local: Local
    # How often it was rerun, and how often it failed then.
    reruns: int
    rerun_failures: int
    # Upstream's status: `flaky`, `failing`, `stable`, `no_data`, or
    # `unknown` when it could not be asked.
    upstream: str

    @property
    def exonerated(self) -> bool:
        """Whether upstream has it flaky or failing too, so the build is not
        to blame for it."""
        return self.upstream in _UPSTREAM_CONFIRMED

    def to_json(self) -> dict:
        return {
            'suite': self.suite,
            'test': self.test,
            'local': self.local.value,
            'reruns': self.reruns,
            'rerun_failures': self.rerun_failures,
            'upstream': self.upstream,
            'exonerated': self.exonerated,
        }


@dataclass(frozen=True)
class Analysis:
    """The verdicts on a build's failing tests."""

    verdicts: tuple[Verdict, ...] = ()

    def exonerate(self, test_run: TestRun) -> TestRun:
        """*test_run* without its exonerated failures. A suite whose
        failures were all exonerated passes."""
        exonerated = {(v.suite, v.test) for v in self.verdicts if v.exonerated}
        suites = []
        for suite in test_run.suites:
            outcome = suite.outcome
            if outcome is None:
                suites.append(suite)
                continue
            kept = tuple(
                f
                for f in outcome.failures
                if (suite.name, f.name) not in exonerated
            )
            if len(kept) == len(outcome.failures):
                suites.append(suite)
                continue
            suites.append(
                dataclasses.replace(
                    suite,
                    passed=suite.passed or not _deterministic(kept),
                    outcome=dataclasses.replace(outcome, failures=kept),
                )
            )
        return dataclasses.replace(test_run, suites=tuple(suites))


def _deterministic(failures: tuple[TestFailure, ...]) -> bool:
    return any(not failure.flaky for failure in failures)


class FlakeAnalysisApi(RecipeApi):
    """Reruns a build's failing tests from their isolates, as upstream
    reruns failures without a CL, and asks LUCI Analysis how they do
    upstream."""

    m: flake_analysis.DEPS

    def __init__(self, properties: InputProperties) -> None:
        super().__init__()
        self._upstream_builders = properties.upstream_builders

    def analyse(
        self, test_run: TestRun, suites: Mapping[str, Rerunnable]
    ) -> Analysis:
        """The verdicts on *test_run*'s failing tests.

        Tests that failed and then passed on the launcher's retry are flaky
        already. The others are rerun from their suite's isolate in *suites*,
        `REPEATS` times each, with no retries and one test per process. Each
        is then checked upstream. The `flake_analysis` output property holds
        the verdicts.

        Args:
            test_run: What the build's suites did.
            suites: The suites that can be rerun, by name.

        Returns:
            The verdicts, which `Analysis.exonerate()` applies to the run.
        """
        failing = [
            suite
            for suite in test_run.suites
            if suite.outcome is not None and suite.outcome.failures
        ]
        if not failing:
            return Analysis()
        verdicts = []
        with self.m.step.nest('flake analysis'):
            for suite in failing:
                with self.m.step.nest(suite.name):
                    local = self._rerun(
                        suite.outcome.failures, suites.get(suite.name)
                    )
                    for failure in suite.outcome.failures:
                        how, reruns, failures = local[failure.name]
                        verdicts.append(
                            Verdict(
                                suite=suite.name,
                                test=failure.name,
                                local=how,
                                reruns=reruns,
                                rerun_failures=failures,
                                upstream=self._upstream(
                                    suite.name, failure.name
                                ),
                            )
                        )
            presentation = self.m.step('verdicts', None).presentation
            presentation.properties['flake_analysis'] = [
                v.to_json() for v in verdicts
            ]
            exonerated = sum(v.exonerated for v in verdicts)
            presentation.step_text = (
                f'{exonerated} of {len(verdicts)} failing tests flaky or '
                'failing upstream too'
            )
        return Analysis(tuple(verdicts))

    def _rerun(
        self,
        failures: tuple[TestFailure, ...],
        rerunnable: Rerunnable | None,
    ) -> dict[str, tuple[Local, int, int]]:
        """How each of *failures* did here, with its reruns and their
        failures."""
        local = {f.name: (Local.FLAKY, 0, 0) for f in failures if f.flaky}
        tests = [f.name for f in failures if not f.flaky]
        if not tests:
            return local
        if rerunnable is None or len(tests) > MAX_RERUN_TESTS:
            return local | {test: (Local.NOT_RERUN, 0, 0) for test in tests}
        summary = self.m.path.mkdtemp('rerun') / 'summary.json'
        args = [
            *(
                arg
                for arg in rerunnable.args
                if not arg.startswith(_FILTER_FILE_FLAG)
            ),
            f'--gtest_filter={":".join(tests)}',
            f'--gtest_repeat={REPEATS}',
            '--test-launcher-retry-limit=0',
            '--test-launcher-batch-limit=1',
            f'--test-launcher-summary-output={summary}',
        ]
        self.m.isolate.run('rerun', rerunnable.isolate, args)
        try:
            data = self.m.file.read_json(
                'read rerun results',
                summary,
                test_data=self.test_api.summary({test: 0 for test in tests}),
            )
        except self.m.file.Error:
            return local | {test: (Local.NOT_RERUN, 0, 0) for test in tests}
        results = {result.name: result for result in read_summary(data)}
        for test in tests:
            result = results.get(test)
            if result is None:
                local[test] = (Local.NOT_RERUN, 0, 0)
                continue
            runs = len(result.runs)
            failed = len(result.failed_runs)
            how = (
                Local.DETERMINISTIC
                if failed == runs
                else Local.FLAKY
                if failed
                else Local.NOT_REPRODUCED
            )
            local[test] = (how, runs, failed)
        return local

    def _upstream(self, suite: str, test: str) -> str:
        """*test*'s status upstream, on the builders comparable to this
        one."""
        builders = self._upstream_builders or UPSTREAM_BUILDERS.get(
            self.m.platform.name, ''
        )
        if not builders:
            return 'unknown'
        result = self.m.step(
            f'upstream {test}',
            [
                self.m.depot_tools.vpython3(),
                self.m.path.brave_core
                / 'tools'
                / 'chromium_tests_analysis'
                / 'is_upstream_flake.py',
                test,
                '--suite',
                f'^{re.escape(suite)}$',
                '--builder',
                builders,
                '--json',
            ],
            check=False,
            stdout=self.m.json.output(),
            step_test_data=lambda: self.test_api.upstream('no_data'),
        )
        if result.retcode not in _UPSTREAM_ANSWERED or not result.stdout:
            return 'unknown'
        statuses = {v['status'] for v in result.stdout.get('variants', [])}
        for status in ('failing', 'flaky', 'stable'):
            if status in statuses:
                result.presentation.step_text = status
                return status
        return 'no_data'
