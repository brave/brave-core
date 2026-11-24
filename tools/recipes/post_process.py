# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Post-process checks for recipe simulation tests.

A recipe's `GenTests` attaches checks with `api.post_process(func, *args)`.
After running a simulation the runner calls each `func(check, steps, *args)`,
where:

  * `check` is a `check.Checker`. Call it as `check(cond)` or
    `check(hint, cond)`. Failures are recorded (not raised) with an
    AST-introspected rendering of the failing expression and its sub-values, so
    every check in a hook runs. See `check.py`.
  * `steps` is an ordered `dict` mapping step name -> step dict
    (`{'name', 'cmd', 'cwd'?, 'env'?, 'retcode'?, 'output_properties'?}`),
    plus the terminal `'$result'` entry. `$result` has no `failure` key on
    success, only a `summaryMarkdown` if the recipe returned one. On a
    non-infra failure it is `{'failure': {'failure': {}, 'humanReason': ...}}`;
    on an infra failure/exception it is `{'failure': {'humanReason': ...}}`.

A check function returns `None` to only assert, or a (subset) `dict` to *filter*
the recorded steps for the written expectation; the runner subset-verifies that
returned dict (see `check.VerifySubset`). Returning an empty dict drops the
expectation file entirely (see `DropExpectation`).
"""

from __future__ import annotations

from collections.abc import Callable, Sequence
import re
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from check import Checker

# The '$result' pseudo-step name holding the overall run result.
RESULT_STEP = '$result'

# A step dict, or the $result dict.
Step = dict
Steps = dict  # ordered {name: Step}


# -- Presence -----------------------------------------------------------------


def MustRun(check: Checker, step_odict: Steps, *steps: str) -> None:
    """Assert that steps with the given names are in the expectations."""
    for step_name in steps:
        check(step_name in step_odict)


def DoesNotRun(check: Checker, step_odict: Steps, *steps: str) -> None:
    """Assert that the given steps don't run."""
    ban_set = set(steps)
    for step_name in step_odict:
        check(step_name not in ban_set)


def MustRunRE(
    check: Checker,
    step_odict: Steps,
    step_regex: str | re.Pattern,
    at_least: int = 1,
    at_most: int | None = None,
) -> None:
    """Assert that steps matching the given regex are in the expectations."""
    compiled_regex = re.compile(step_regex)
    matches = 0
    for step_name in step_odict:
        if compiled_regex.match(step_name):
            matches += 1
    check(matches >= at_least)
    if at_most is not None:
        check(matches <= at_most)


def DoesNotRunRE(check: Checker, step_odict: Steps, *step_regexes: str) -> None:
    """Assert that no steps matching any of the regexes have run."""
    compiled_regexes = [re.compile(r) for r in step_regexes]
    for step_name in step_odict:
        for regex in compiled_regexes:
            check(not regex.match(step_name))


# -- Command inspection -------------------------------------------------------


def _fullmatch(pattern: str | re.Pattern, string: str) -> bool:
    match = re.match(pattern, string)
    return bool(match and match.span()[1] == len(string))


def _is_subsequence(sub: list, seq: list) -> bool:
    """Whether *sub* appears in *seq* in order (not necessarily contiguously)."""
    it = iter(seq)
    return all(item in it for item in sub)


def StepCommandContains(
    check: Checker,
    step_odict: Steps,
    step: str,
    argument_sequence: Sequence[str],
) -> None:
    """Assert that a step's command contained the given argument sequence."""
    check(
        'command line for step %s contained %s' % (step, argument_sequence),
        _is_subsequence(
            [str(a) for a in argument_sequence], step_odict[step]['cmd']
        ),
    )


def StepCommandDoesNotContain(
    check: Checker,
    step_odict: Steps,
    step: str,
    argument_sequence: Sequence[str],
) -> None:
    """Assert that a step's command did not contain the given argument
    sequence."""
    check(
        'command line for step %s did not contain %s'
        % (step, argument_sequence),
        not _is_subsequence(
            [str(a) for a in argument_sequence], step_odict[step]['cmd']
        ),
    )


def StepCommandRE(
    check: Checker,
    step_odict: Steps,
    step: str,
    expected_patterns: Sequence[str | re.Pattern],
) -> None:
    """Assert that a step's command matches the given list of regexes.

    The i-th command argument is matched against the i-th pattern. A pattern
    that does not match the entire argument is a failure, as are surplus
    arguments or unused patterns.
    """
    cmd = step_odict[step]['cmd']
    for expected, actual in zip(expected_patterns, cmd):
        check(_fullmatch(expected, actual))
    unmatched = cmd[len(expected_patterns) :]
    check('all arguments matched', not unmatched)
    unused = expected_patterns[len(cmd) :]
    check('all patterns used', not unused)


# -- Per-step status ----------------------------------------------------------


def StepSuccess(check: Checker, step_odict: Steps, step: str) -> None:
    """Assert that a step succeeded (its simulated retcode was zero)."""
    check(step_odict[step].get('retcode', 0) == 0)


def StepFailure(check: Checker, step_odict: Steps, step: str) -> None:
    """Assert that a step failed (its simulated retcode was non-zero)."""
    check(step_odict[step].get('retcode', 0) != 0)


# -- Output properties --------------------------------------------------------


def GetBuildProperties(step_odict: Steps) -> dict[str, Any]:
    """Retrieves the build properties for a recipe."""
    build_properties = {}
    for name, step in step_odict.items():
        if name == RESULT_STEP:
            continue
        for prop, value in step.get('output_properties', {}).items():
            build_properties[prop] = value
    return build_properties


def PropertyEquals(
    check: Checker, step_odict: Steps, key: str, value: Any
) -> None:
    """Assert that a recipe's output property `key` equals `value`.

    Args:
      key (str) - The key to look for in output properties.
      value (jsonish) - The value to look for in output properties.

    Usage:
      yield api.test(
          ..., api.post_process(PropertyEquals, 'do_not_retry', True)
      )
    """
    build_properties = GetBuildProperties(step_odict)

    if check(key in build_properties):
        check(build_properties[key] == value)


def PropertyMatchesRE(
    check: Checker,
    step_odict: Steps,
    key: str,
    pattern: str | re.Pattern,
) -> None:
    """Assert that a recipe's output property `key` value matches `pattern`.

    Args:
      key - The key to look for in output properties.
      pattern - The pattern for comparison.

    Usage:
      yield api.test(
          ...,
          api.post_process(PropertyMatchesRE, 'key', r'.*value.*'),
      )
    """
    build_properties = GetBuildProperties(step_odict)

    if check(key in build_properties):
        if check(isinstance(build_properties[key], str)):
            check(re.search(pattern, build_properties[key]))


def PropertyMatchesCallable(
    check: Checker,
    step_odict: Steps,
    key: str,
    matcher: Callable[[Checker, Any], bool],
) -> None:
    """Assert that a recipe's output property `key` meets conditions of
    `matcher`.

    Args:
      key - The key to look for in output properties.
      matcher - A callable that evaluates the property.

    Usage:
      yield api.test(
          ...,
          api.post_process(
              PropertyMatchesCallable,
              'key',
              lambda check, x: check('foo' in x),
          ),
      )
    """
    build_properties = GetBuildProperties(step_odict)

    if check(key in build_properties):
        check(matcher(check, build_properties[key]))


def PropertiesContain(check: Checker, step_odict: Steps, key: str) -> None:
    """Assert that a recipe's output properties contain `key`.

    Args:
      key (str) - The key to check for.

    Usage:
      yield api.test(..., api.post_process(PropertiesContain, 'key'))
    """
    build_properties = GetBuildProperties(step_odict)

    check(key in build_properties)


def PropertiesDoNotContain(check: Checker, step_odict: Steps, key: str) -> None:
    """Assert that a recipe's output properties do not contain `key`.

    Args:
      key (str) - The key to check for.

    Usage:
      yield api.test(..., api.post_process(PropertiesDoNotContain, 'key'))
    """
    build_properties = GetBuildProperties(step_odict)

    check(key not in build_properties)


# -- Overall result -----------------------------------------------------------


def StatusSuccess(check: Checker, step_odict: Steps) -> None:
    """Assert that the recipe finished successfully."""
    failure = step_odict[RESULT_STEP].get('failure')
    check('recipe succeeded (found failure instead)', failure is None)


def StatusAnyFailure(check: Checker, step_odict: Steps) -> None:
    """Assert that the recipe failed (infra or non-infra)."""
    check(
        'recipe failed (found success instead)',
        'failure' in step_odict[RESULT_STEP],
    )


def StatusFailure(check: Checker, step_odict: Steps) -> None:
    """Assert that the recipe had a non-infra failure."""
    result = step_odict[RESULT_STEP]
    if not check('recipe failed (found success instead)', 'failure' in result):
        return
    check(
        'expected failure but recipe had infra failure',
        'failure' in result['failure'],
    )


def StatusException(check: Checker, step_odict: Steps) -> None:
    """Assert that the recipe had an infra failure."""
    result = step_odict[RESULT_STEP]
    if not check(
        'recipe had infra failure (found success instead)', 'failure' in result
    ):
        return
    check(
        'recipe had infra failure (found non-infra failure instead)',
        'failure' not in result['failure'],
    )


def SummaryMarkdown(check: Checker, step_odict: Steps, summary: str) -> None:
    """Assert that recipe output summary is the same as the given summary.

    Args:
      summary (str): the string to match.

    Usage:
      yield api.test(..., api.post_process(SummaryMarkdown, 'summary'))
    """
    result = step_odict[RESULT_STEP]
    actual_summary = result.get('failure', {}).get('humanReason') or result.get(
        'summaryMarkdown'
    )
    if not check("recipe doesn't output any summary", actual_summary):
        return
    check(
        'expected recipe output summary %r (found summary %r instead)'
        % (summary, actual_summary),
        summary == actual_summary,
    )


def SummaryMarkdownRE(
    check: Checker, step_odict: Steps, summary_regex: str
) -> None:
    """Assert that recipe output summary matches given regex.

    Args:
      summary_regex (str): the regular expression to match.

    Usage:
      yield api.test(..., api.post_process(SummaryMarkdownRE, 'summary: .*'))
    """
    result = step_odict[RESULT_STEP]
    actual_summary = result.get('failure', {}).get('humanReason') or result.get(
        'summaryMarkdown'
    )
    if not check("recipe doesn't output any summary", actual_summary):
        return
    check(
        'expected recipe output summary matches %r (found summary %r instead)'
        % (summary_regex, actual_summary),
        re.search(summary_regex, actual_summary),
    )


# -- Expectation control ------------------------------------------------------


def DropExpectation(_check: Checker, _step_odict: Steps) -> Steps:
    """Suppress writing this test's expectation file.

    Returning an empty mapping signals the runner to write no expectation (and,
    in train mode, to remove any stale one). Handy for tests that assert only via
    other post-process checks and don't need a golden file. Must be the last
    check in a case: no steps remain for later checks to evaluate.
    """
    return {}
