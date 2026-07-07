# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Simulation-test runner for Brave recipes (`engine.py test run|train`).

Discovers every testable recipe -- those under `recipes/` and the module
example/test recipes under `recipe_modules/<mod>/{examples,tests}/` that define
both `RunSteps` and `GenTests` -- then, for each `TestData` a recipe's
`GenTests` yields:

  1. builds a `TestContext` from the case's seed values,
  2. runs the recipe under a fresh engine in test mode (no real I/O),
  3. records the ordered step stream and the run's `$result` status,
  4. applies the case's `post_process` checks,
  5. compares the result against the checked-in expectation JSON (`run`) or
     writes it (`train`).

Modules are tested the recipes_py way: via small example recipes that exercise
them, run through this very machinery.
"""

from __future__ import annotations

import argparse
import difflib
import importlib
import json
import logging
from pathlib import Path
import re
import subprocess
import sys

import engine
import simulation
from recipe_test_api import RecipeTestApi, TestData

# -- TEST_API discovery & injection (mirrors engine's DEPS resolution) --------


def _find_test_api_class(module, name: str) -> type[RecipeTestApi]:
    """Return the single `RecipeTestApi` subclass in *module*, if any."""
    classes = [
        value for value in vars(module).values()
        if isinstance(value, type) and issubclass(value, RecipeTestApi)
        and value is not RecipeTestApi and value.__module__ == module.__name__
    ]
    if len(classes) > 1:
        raise RuntimeError(
            f"recipe module '{name}' defines {len(classes)} RecipeTestApi "
            f'subclasses in test_api.py; expected at most one')
    return classes[0] if classes else RecipeTestApi


def _instantiate_test_module(name: str, chain: list[str],
                             cache: dict[str, RecipeTestApi]) -> RecipeTestApi:
    """Instantiate a module's TEST_API, wiring its DEPS onto `.m` (cached)."""
    if name in cache:
        return cache[name]
    if name in chain:
        cycle = ' -> '.join(chain + [name])
        raise RuntimeError(f'cyclical DEPS detected: {cycle}')

    package = importlib.import_module(f'{engine.MODULES_PKG}.{name}')
    deps = list(getattr(package, 'DEPS', []))

    test_api_path = (engine.RECIPES_ROOT / engine.MODULES_PKG / name /
                     'test_api.py')
    if test_api_path.exists():
        test_module = importlib.import_module(
            f'{engine.MODULES_PKG}.{name}.test_api')
        api_class = _find_test_api_class(test_module, name)
    else:
        # Modules without a test_api.py contribute the base api (no helpers).
        api_class = RecipeTestApi

    inst = api_class(module=name)
    for dep_name in deps:
        setattr(inst.m, dep_name,
                _instantiate_test_module(dep_name, chain + [name], cache))
    setattr(inst.m, name, inst)
    cache[name] = inst
    return inst


def _build_root_test_api(deps: list[str]) -> RecipeTestApi:
    """Build the `api` passed to `GenTests`, with each DEP injected by name."""
    root = RecipeTestApi(module=None)
    cache: dict[str, RecipeTestApi] = {}
    for dep_name in deps:
        setattr(root, dep_name, _instantiate_test_module(dep_name, [], cache))
    return root


# -- Recipe discovery ---------------------------------------------------------


def _iter_recipe_ids() -> list[str]:
    """Every candidate recipe id, from `recipes/` and module examples/tests."""
    root = engine.RECIPES_ROOT
    ids: list[str] = []

    recipes_root = root / engine.RECIPES_PKG
    for path in sorted(recipes_root.rglob('*.py')):
        if path.name == '__init__.py':
            continue
        ids.append(path.relative_to(recipes_root).with_suffix('').as_posix())

    modules_root = root / engine.MODULES_PKG
    for module in sorted(engine._module_names()):  # pylint: disable=protected-access
        for sub in ('examples', 'tests'):
            directory = modules_root / module / sub
            if not directory.is_dir():
                continue
            for path in sorted(directory.rglob('*.py')):
                if path.name == '__init__.py':
                    continue
                ids.append(
                    path.relative_to(modules_root).with_suffix('').as_posix())
    return ids


def _testable_recipes() -> list[tuple[str, object]]:
    """(id, module) for every recipe defining both RunSteps and GenTests."""
    recipes = []
    for recipe_id in _iter_recipe_ids():
        recipe = engine._import_recipe(recipe_id)  # pylint: disable=protected-access
        if hasattr(recipe, 'RunSteps') and hasattr(recipe, 'GenTests'):
            recipes.append((recipe_id, recipe))
    return recipes


def _expect_dir(recipe) -> Path:
    recipe_file = Path(recipe.__file__)
    return recipe_file.parent / f'{recipe_file.stem}.expected'


def _expect_file(recipe, test_name: str) -> Path:
    safe = re.sub(r'[^A-Za-z0-9._-]', '_', test_name)
    return _expect_dir(recipe) / f'{safe}.json'


def _dump(steps: dict[str, dict]) -> str:
    return json.dumps(list(steps.values()), indent=2, sort_keys=True) + '\n'


def _read_expectation(path: Path) -> str:
    """Read an expectation file as UTF-8 with no newline translation.

    Expectations are compared byte-for-byte and must be identical on every
    platform, so we avoid `read_text`'s locale-encoding and universal-newline
    defaults (see tools/cr/python_dos_and_donts.md).
    """
    return path.read_bytes().decode('utf-8')


def _write_expectation(path: Path, text: str) -> None:
    """Write an expectation file as UTF-8 with no newline translation."""
    path.write_text(text, encoding='utf-8', newline='')


def _rel(path: Path) -> str:
    try:
        return str(path.relative_to(engine.RECIPES_ROOT))
    except ValueError:
        return str(path)


# -- Running ------------------------------------------------------------------


class _Results:
    """Accumulates per-case verdicts for the final report."""

    def __init__(self) -> None:
        self.passed = 0
        self.written = 0
        self.removed = 0
        self.failures: list[tuple[str, list[str]]] = []

    def record(self, case_id: str, failures: list[str]) -> None:
        if failures:
            self.failures.append((case_id, failures))
        else:
            self.passed += 1


def _simulate(
        recipe, recipe_id: str,
        test_data: TestData) -> tuple[dict[str, dict], simulation.TestContext]:
    """Run one recipe case in a fresh test-mode engine; return (steps, ctx)."""
    ctx = simulation.TestContext.from_test_data(test_data)
    # A fresh engine per case: module instances are cached per engine, so
    # reusing one would leak state (deployed depot_tools, set env vars, ...).
    eng = engine._Engine(  # pylint: disable=protected-access
        brave_core_ref=test_data.brave_core_ref,
        test=ctx)
    status, reason = 'SUCCESS', None
    try:
        eng.run_loaded_recipe(recipe, recipe_id, test_data.properties)
    except subprocess.CalledProcessError:
        # A checked step returned non-zero: the recipe failed as designed.
        status = 'FAILURE'
    except Exception as exc:  # pylint: disable=broad-except
        # Any other exception is an infra/logic error (bad input, missing dep).
        status, reason = 'EXCEPTION', str(exc)
    steps = simulation.build_steps(ctx.step_runner, status, reason, ctx)
    return steps, ctx


def _run_case(recipe, recipe_id: str, test_data: TestData, train: bool,
              results: _Results, seen: set[Path]) -> None:
    steps, _ = _simulate(recipe, recipe_id, test_data)
    filtered, failures = simulation.apply_post_process(
        test_data.post_process_hooks, steps)

    status = steps[simulation.pp.RESULT_STEP]['status']
    if (test_data.expected_status is not None
            and test_data.expected_status != status):
        reason = steps[simulation.pp.RESULT_STEP].get('reason')
        failures.append(f'expected overall status '
                        f'{test_data.expected_status!r}, got {status!r}' +
                        (f' ({reason})' if reason else ''))

    expect_file = _expect_file(recipe, test_data.name)
    seen.add(expect_file)

    if filtered is None:  # DropExpectation.
        if expect_file.exists():
            if train:
                expect_file.unlink()
                results.removed += 1
            else:
                failures.append(
                    f'{_rel(expect_file)} exists but the test drops its '
                    f'expectation; delete it or run `test train`')
    else:
        payload = _dump(filtered)
        if train:
            expect_file.parent.mkdir(parents=True, exist_ok=True)
            if (not expect_file.exists()
                    or _read_expectation(expect_file) != payload):
                _write_expectation(expect_file, payload)
                results.written += 1
        elif not expect_file.exists():
            failures.append(f'missing expectation {_rel(expect_file)} '
                            f'(run `engine.py test train`)')
        else:
            current = _read_expectation(expect_file)
            if current != payload:
                diff = difflib.unified_diff(
                    current.splitlines(),
                    payload.splitlines(),
                    fromfile=f'{_rel(expect_file)} (expected)',
                    tofile='actual',
                    lineterm='')
                failures.append('expectation mismatch:\n' + '\n'.join(diff))

    results.record(f'{recipe_id}.{test_data.name}', failures)


def run_tests(train: bool = False,
              filter_: str | None = None,
              list_only: bool = False) -> int:
    """Run (or train, or list) all recipe simulation tests; return an exit code.
    """
    engine._ensure_on_sys_path()  # pylint: disable=protected-access
    results = _Results()

    for recipe_id, recipe in _testable_recipes():
        root_api = _build_root_test_api(list(getattr(recipe, 'DEPS', [])))
        seen: set[Path] = set()
        for test_data in recipe.GenTests(root_api):
            case_id = f'{recipe_id}.{test_data.name}'
            if filter_ and filter_ not in case_id:
                continue
            if list_only:
                print(case_id)
                continue
            _run_case(recipe, recipe_id, test_data, train, results, seen)

        # Train mode prunes stale expectation files (only on a full, unfiltered
        # run, so a filtered train never deletes another case's golden).
        if train and not list_only and not filter_:
            expect_dir = _expect_dir(recipe)
            if expect_dir.is_dir():
                for stale in sorted(expect_dir.glob('*.json')):
                    if stale not in seen:
                        stale.unlink()
                        results.removed += 1

    if list_only:
        return 0
    return _report(results, train)


def _report(results: _Results, train: bool) -> int:
    for case_id, failures in results.failures:
        print(f'FAIL {case_id}')
        for failure in failures:
            for line in failure.splitlines():
                print(f'    {line}')
    summary = [f'{results.passed} passed', f'{len(results.failures)} failed']
    if train:
        summary.append(f'{results.written} written')
        summary.append(f'{results.removed} removed')
    print(f'\nrecipe tests: {", ".join(summary)}')
    return 1 if results.failures else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog='engine.py test',
        description='Run Brave recipe simulation tests.')
    parser.add_argument('subcommand',
                        choices=['run', 'train', 'list'],
                        help='run: compare against expectations; train: write '
                        'expectations; list: print test case ids')
    parser.add_argument('--filter',
                        default=None,
                        help='only cases whose "<recipe>.<test>" id contains '
                        'this substring')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='enable step/debug logging (noisy)')
    args = parser.parse_args(argv)

    # Keep step logging quiet by default so test output is just the report.
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.WARNING)
    return run_tests(train=args.subcommand == 'train',
                     filter_=args.filter,
                     list_only=args.subcommand == 'list')


if __name__ == '__main__':
    sys.exit(main())
