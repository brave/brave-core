# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Post-process checks for recipe simulation tests.

A minimal mirror of `recipe_engine.post_process`. A recipe's `GenTests` attaches
checks with `api.post_process(func, *args)`; after simulation the test runner
calls each `func(check, steps, *args)`, where:

  * `check` is a `Checker`. Call it as `check(cond)` or `check(hint, cond)`.
    Failures are collected (not raised), so every check in a hook runs.
  * `steps` is an ordered `dict` mapping step name -> step dict
    (`{'name', 'cmd', 'cwd'?, 'env'?, 'retcode'}`), plus the terminal
    `'$result'` entry (`{'name': '$result', 'status': ...}`).

A check function returns `None` to only assert, or a (subset) `dict` to *filter*
the recorded steps for the written expectation. Returning an empty dict drops
the expectation file entirely (see `DropExpectation`).

Simplifications vs upstream: no AST-introspected failure rendering (checks carry
a caller-supplied hint), and the returned-dict filter is used verbatim rather
than subset-verified.
"""

from __future__ import annotations

import re
from typing import Any

# The '$result' pseudo-step name holding the overall run status.
RESULT_STEP = '$result'

# A step dict, or the $result dict.
Step = dict
Steps = dict  # OrderedDict-like {name: Step}

# Sentinel distinguishing `check(cond)` from `check(hint, cond)`.
_MISSING = object()


class Checker:
    """Collects check failures for a single post-process hook.

    Calling the checker records (but does not raise) failures, so a hook reports
    every failed assertion in one pass. `failures` is a list of human-readable
    strings, each prefixed with the `api.post_process(...)` call site.
    """

    def __init__(self, context: str) -> None:
        self._context = context
        self.failures: list[str] = []

    def __call__(self, arg1: Any, arg2: Any = _MISSING) -> bool:
        """`check(cond)` or `check(hint, cond)`; returns `bool(cond)`."""
        if arg2 is _MISSING:
            hint, cond = None, arg1
        else:
            hint, cond = arg1, arg2
        if not cond:
            message = f'{self._context}: check failed'
            if hint:
                message += f': {hint}'
            self.failures.append(message)
        return bool(cond)


def _run_steps(steps: Steps) -> Steps:
    """Return only the executed steps (drops the `$result` pseudo-step)."""
    return {name: step for name, step in steps.items() if name != RESULT_STEP}


# -- Presence -----------------------------------------------------------------


def MustRun(check: Checker, steps: Steps, step_name: str) -> None:
    check(f'step {step_name!r} must run', step_name in _run_steps(steps))


def DoesNotRun(check: Checker, steps: Steps, step_name: str) -> None:
    check(f'step {step_name!r} must not run', step_name
          not in _run_steps(steps))


def MustRunRE(check: Checker, steps: Steps, pattern: str) -> None:
    regex = re.compile(pattern)
    check(f'a step matching {pattern!r} must run',
          any(regex.search(name) for name in _run_steps(steps)))


def DoesNotRunRE(check: Checker, steps: Steps, pattern: str) -> None:
    regex = re.compile(pattern)
    check(f'no step may match {pattern!r}',
          not any(regex.search(name) for name in _run_steps(steps)))


# -- Command inspection -------------------------------------------------------


def _is_subsequence(sub: list, seq: list) -> bool:
    """Whether *sub* appears in *seq* in order (not always contiguously)."""
    it = iter(seq)
    return all(item in it for item in sub)


def StepCommandContains(check: Checker, steps: Steps, step_name: str,
                        args: list[str]) -> None:
    step = _run_steps(steps).get(step_name)
    if not check(f'step {step_name!r} must run', step is not None):
        return
    check(f'{step_name!r} command must contain {args!r} in order',
          _is_subsequence([str(a) for a in args], step['cmd']))


def StepCommandRE(check: Checker, steps: Steps, step_name: str,
                  patterns: list[str]) -> None:
    step = _run_steps(steps).get(step_name)
    if not check(f'step {step_name!r} must run', step is not None):
        return
    cmd = step['cmd']
    for i, pattern in enumerate(patterns):
        matched = i < len(cmd) and re.search(pattern, cmd[i]) is not None
        check(f'{step_name!r} cmd[{i}] must match {pattern!r}', matched)


# -- Per-step status ----------------------------------------------------------


def StepSuccess(check: Checker, steps: Steps, step_name: str) -> None:
    step = _run_steps(steps).get(step_name)
    if not check(f'step {step_name!r} must run', step is not None):
        return
    check(f'step {step_name!r} must succeed', step.get('retcode', 0) == 0)


def StepFailure(check: Checker, steps: Steps, step_name: str) -> None:
    step = _run_steps(steps).get(step_name)
    if not check(f'step {step_name!r} must run', step is not None):
        return
    check(f'step {step_name!r} must fail', step.get('retcode', 0) != 0)


# -- Overall result -----------------------------------------------------------


def _status(steps: Steps) -> str | None:
    result = steps.get(RESULT_STEP)
    return result.get('status') if result else None


def StatusSuccess(check: Checker, steps: Steps) -> None:
    check(f'overall status must be SUCCESS (was {_status(steps)})',
          _status(steps) == 'SUCCESS')


def StatusFailure(check: Checker, steps: Steps) -> None:
    check(f'overall status must be FAILURE (was {_status(steps)})',
          _status(steps) == 'FAILURE')


def StatusException(check: Checker, steps: Steps) -> None:
    check(f'overall status must be EXCEPTION (was {_status(steps)})',
          _status(steps) == 'EXCEPTION')


# -- Expectation control ------------------------------------------------------


def DropExpectation(_check: Checker, _steps: Steps) -> Steps:
    """Suppress writing this test's expectation file.

    Returning an empty mapping signals the runner to write (and, in train mode,
    remove any stale) expectation file. Handy for tests that assert only via
    other post-process checks and don't need a golden file.
    """
    return {}
