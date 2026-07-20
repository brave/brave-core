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

Modules are tested via small example recipes that exercise them, run through
this very machinery.
"""

from __future__ import annotations

import argparse
import difflib
import importlib
import io
import json
import logging
from pathlib import Path
import re
import subprocess
import sys
import time
from typing import TYPE_CHECKING

import engine
import simulation
from recipe_test_api import RecipeTestApi, TestData

if TYPE_CHECKING:
    import coverage

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


# A failure "block": its kind (grouped under one `FAIL (<kind>) - <case>`
# header) and the lines that describe it.
Block = tuple[str, list[str]]


class _Results:
    """Accumulates per-case verdicts for the final report."""

    def __init__(self, verbose: bool = False) -> None:
        self.verbose = verbose
        self.passed = 0
        self.total = 0
        self.written = 0
        self.removed = 0
        self.failures: list[tuple[str, list[Block]]] = []

    def record(self, case_id: str, blocks: list[Block]) -> None:
        """Record a finished case, print its progress icon, buffer failures."""
        self.total += 1
        failed = bool(blocks)
        if failed:
            self.failures.append((case_id, blocks))
        else:
            self.passed += 1
        if self.verbose:
            print(f'{case_id} ... {"FAIL" if failed else "ok"}')
        else:
            sys.stdout.write('F' if failed else '.')
            sys.stdout.flush()


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
    # `failure` becomes the `$result` payload: None on success; a non-infra
    # failure carries an inner `{'failure': {}}`; an infra failure/exception
    # carries only `humanReason`.
    failure: dict | None = None
    try:
        eng.run_loaded_recipe(recipe, recipe_id, test_data.properties)
    except subprocess.CalledProcessError:
        # A checked step returned non-zero: the recipe failed as designed. The
        # failing step is the last one the runner recorded before it raised.
        recorded = ctx.step_runner.recorded_steps
        name = recorded[-1]['name'] if recorded else '<unknown>'
        failure = {'failure': {}, 'humanReason': f'Step({name!r}) failed'}
    except Exception as exc:  # pylint: disable=broad-except
        # Any other exception is an infra/logic error (bad input, missing dep).
        failure = {'humanReason': str(exc)}
    steps = simulation.build_steps(ctx.step_runner, failure, ctx)
    return steps, ctx


def _result_status(result: dict) -> str:
    """Derive the SUCCESS/FAILURE/EXCEPTION label from a `$result` dict."""
    failure = result.get('failure')
    if failure is None:
        return 'SUCCESS'
    return 'FAILURE' if 'failure' in failure else 'EXCEPTION'


def _run_case(recipe, recipe_id: str, test_data: TestData, train: bool,
              results: _Results, seen: set[Path]) -> None:
    case_id = f'{recipe_id}.{test_data.name}'
    steps, _ = _simulate(recipe, recipe_id, test_data)
    expect_file = _expect_file(recipe, test_data.name)
    seen.add(expect_file)

    blocks: list[Block] = []
    try:
        filtered, failed_checks = simulation.apply_post_process(
            test_data.post_process_hooks, steps)
    except simulation.PostProcessError as exc:
        # A hook returned a non-subset expectation: the test itself is broken.
        results.record(case_id, [('bad_test', [str(exc)])])
        return
    blocks.extend(('check', chk.format()) for chk in failed_checks)

    result = steps[simulation.pp.RESULT_STEP]
    status = _result_status(result)
    if (test_data.expected_status is not None
            and test_data.expected_status != status):
        reason = (result.get('failure') or {}).get('humanReason')
        blocks.append(('bad_test', [
            f'expected overall status {test_data.expected_status!r}, '
            f'got {status!r}' + (f' ({reason})' if reason else '')
        ]))

    if filtered is None:  # DropExpectation.
        if expect_file.exists():
            if train:
                expect_file.unlink()
                results.removed += 1
            else:
                blocks.append(('bad_test', [
                    f'{_rel(expect_file)} exists but the test drops its '
                    f'expectation; delete it or run `test train`'
                ]))
    else:
        payload = _dump(filtered)
        if train:
            expect_file.parent.mkdir(parents=True, exist_ok=True)
            if (not expect_file.exists()
                    or _read_expectation(expect_file) != payload):
                _write_expectation(expect_file, payload)
                results.written += 1
        elif not expect_file.exists():
            blocks.append(('bad_test', [
                f'missing expectation {_rel(expect_file)} '
                f'(run `engine.py test train`)'
            ]))
        else:
            current = _read_expectation(expect_file)
            if current != payload:
                diff = difflib.unified_diff(
                    current.splitlines(),
                    payload.splitlines(),
                    fromfile=f'{_rel(expect_file)} (expected)',
                    tofile='actual',
                    lineterm='')
                blocks.append(('diff', list(diff)))

    results.record(case_id, blocks)


# -- Coverage & structural gates ----------------------------------------------


def _start_coverage() -> coverage.Coverage:
    """Start a run-wide `coverage` session over recipe/module source.

    Must be called before recipes/modules are imported so their import-time
    definitions count as covered. Returns the started `Coverage` object.
    """
    import coverage  # Local import: only the full-run path needs the wheel.
    root = engine.RECIPES_ROOT
    cov = coverage.Coverage(config_file=False,
                            data_file=None,
                            include=[
                                str(root / engine.RECIPES_PKG / '*'),
                                str(root / engine.MODULES_PKG / '*'),
                            ])
    # `if TYPE_CHECKING:` blocks never execute at runtime; the default
    # `# pragma: no cover` still applies for the production I/O seams that
    # simulation intentionally never exercises.
    cov.exclude(r'^\s*if TYPE_CHECKING:')
    cov.start()
    return cov


def _report_coverage(cov: coverage.Coverage) -> bool:
    """Report total coverage; return True if it is below 100% (a failure)."""
    import coverage  # Local import: mirrors `_start_coverage`.
    if not cov.get_data().measured_files():
        return False
    buf = io.StringIO()
    pct = 0
    try:
        pct = cov.report(file=buf, show_missing=True, skip_covered=True)
    except coverage.CoverageException as ex:
        print('%s: %s' % (ex.__class__.__name__, ex))
    if int(pct) != 100:
        print(buf.getvalue())
        print('FATAL: Insufficient total coverage (%.2f%%)' % pct)
        print()
        return True
    return False


def _uncovered_modules() -> list[str]:
    """Modules with no test coverage at all (no examples/tests, not exempt)."""
    modules_root = engine.RECIPES_ROOT / engine.MODULES_PKG
    uncovered: list[str] = []
    for module in sorted(engine._module_names()):  # pylint: disable=protected-access
        mod_dir = modules_root / module
        has_tests = any((mod_dir / sub).is_dir() and any(
            p.name != '__init__.py' for p in (mod_dir / sub).rglob('*.py'))
                        for sub in ('examples', 'tests'))
        if has_tests:
            continue
        package = importlib.import_module(f'{engine.MODULES_PKG}.{module}')
        if getattr(package, 'DISABLE_STRICT_COVERAGE', False):
            continue
        uncovered.append(module)
    return uncovered


def _all_expectation_files() -> list[Path]:
    root = engine.RECIPES_ROOT
    files: list[Path] = []
    for base in (engine.RECIPES_PKG, engine.MODULES_PKG):
        for expect_dir in (root / base).rglob('*.expected'):
            if expect_dir.is_dir():
                files.extend(expect_dir.glob('*.json'))
    return files


# -- Running ------------------------------------------------------------------


def run_tests(train: bool = False,
              filter_: str | None = None,
              list_only: bool = False,
              verbose: bool = False) -> int:
    """Run (or train, or list) all recipe simulation tests; return an exit code.
    """
    engine._ensure_on_sys_path()  # pylint: disable=protected-access
    results = _Results(verbose=verbose)

    # Coverage/structural gates run only on a full, unfiltered `run`/`train`.
    # Start coverage before any recipe/module import so import-time definitions
    # are measured.
    gated = not filter_ and not list_only
    cov = _start_coverage() if gated else None

    start = time.monotonic()
    seen_all: set[Path] = set()
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
        if train and gated:
            expect_dir = _expect_dir(recipe)
            if expect_dir.is_dir():
                for stale in sorted(expect_dir.glob('*.json')):
                    if stale not in seen:
                        stale.unlink()
                        results.removed += 1
        seen_all |= seen

    if list_only:
        return 0

    duration = time.monotonic() - start
    if cov is not None:
        cov.stop()

    # `run` reports orphaned goldens; `train` already deleted them above.
    unused = ([] if train else sorted(p for p in _all_expectation_files()
                                      if p not in seen_all)) if gated else []
    uncovered = _uncovered_modules() if gated else []
    return _report(results, train, duration, cov, uncovered, unused)


def _print_case_failure(case_id: str, blocks: list[Block]) -> None:
    """Print one failed case's detail, grouped by kind."""
    by_kind: dict[str, list[list[str]]] = {}
    for kind, lines in blocks:
        by_kind.setdefault(kind, []).append(lines)
    for kind in ('bad_test', 'check', 'diff'):
        groups = by_kind.get(kind)
        if not groups:
            continue
        print('=' * 70)
        print(f'FAIL ({kind}) - {case_id}')
        print('-' * 70)
        for lines in groups:
            for line in lines:
                print(line)
            print()


def _report(results: _Results, train: bool, duration: float,
            cov: coverage.Coverage | None, uncovered: list[str],
            unused: list[Path]) -> int:
    if not results.verbose:
        print()  # Terminate the progress-dots line.
    for case_id, blocks in results.failures:
        _print_case_failure(case_id, blocks)

    print('-' * 70)
    print('Ran %d tests in %0.3fs' % (results.total, duration))
    print()

    fail = bool(results.failures)

    if cov is not None:
        fail = _report_coverage(cov) or fail

    if uncovered:
        fail = True
        print('------')
        print('ERROR: The following modules lack any form of test coverage:')
        for modname in uncovered:
            print('  ', modname)
        print()

    if unused:
        fail = True
        print('------')
        print(
            'ERROR: The below expectation files have no associated test case:')
        for path in unused:
            print('  ', _rel(path))
        print()

    summary = [f'{results.passed} passed', f'{len(results.failures)} failed']
    if train:
        summary.append(f'{results.written} written')
        summary.append(f'{results.removed} removed')
    print(f'recipe tests: {", ".join(summary)}')
    return 1 if fail else 0


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
                     list_only=args.subcommand == 'list',
                     verbose=args.verbose)


if __name__ == '__main__':
    sys.exit(main())
