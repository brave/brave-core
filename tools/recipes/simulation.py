# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Simulation runtime for recipe tests.

Holds everything needed to run a recipe with zero real side effects:

  * `TestContext`: run-global simulated state (env, filesystem, `which`
    resolution, platform, home) plus the step runner. Built from a `TestData`
    via `TestContext.from_test_data`. The engine seeds one onto each module (as
    `RecipeApi._test`); the seam modules (`path`, `env`, `platform`, `step`)
    read/mutate it instead of the real machine.
  * `SubprocessStepRunner` / `SimulationStepRunner`: the two `step`
    back-ends. Production shells out (with the Windows resolution that used to
    live in `StepApi`); simulation records an ordered step list and returns
    canned results.
  * expectation helpers: `stabilize` (machine paths -> `[WORKSPACE]`/`[HOME]`
    tokens), `build_steps`, and `apply_post_process`.

Deliberately never imports `engine` (the engine imports this), so there is no
cycle: the test runner in `engine.py` drives a recipe, then hands the recorded
steps here for post-processing and serialization.
"""

from __future__ import annotations

from collections.abc import Iterable
import copy
import inspect
from pathlib import Path, PurePosixPath
import subprocess
import sys
from typing import Any

from check import Check, Checker, PostProcessError, VerifySubset
from engine_env import merge_envs
import post_process as pp
from recipe_test_api import PostprocessHookContext, StepTestData, TestData

# Fixed synthetic locations used in test mode. They are never touched on disk;
# they exist only so module-derived paths are deterministic, and are rewritten
# to tokens in expectations so goldens are machine-independent.
SIM_WORKSPACE = PurePosixPath('/b/s')
SIM_HOME = PurePosixPath('/b/home')

WORKSPACE_TOKEN = '[WORKSPACE]'
HOME_TOKEN = '[HOME]'


def _norm(path: str | Path) -> str:
    """Normalize a path to a stable string key for the simulated filesystem."""
    return str(PurePosixPath(str(path)))


class SimFS:
    """A tiny simulated filesystem: sets of files and directories.

    Seeded from `api.path.files(...)` / `api.path.dirs(...)` and mutated by
    `api.path.mkdir(...)` during a run. A directory also "exists" implicitly if
    it is an ancestor of any seeded/created path, so parents of seeded files
    don't each need to be declared.
    """

    def __init__(
        self,
        files: Iterable[str | Path] = (),
        dirs: Iterable[str | Path] = ()
    ) -> None:
        self._files: set[str] = {_norm(f) for f in files}
        self._dirs: set[str] = {_norm(d) for d in dirs}

    def _has_descendant(self, path: str) -> bool:
        prefix = path.rstrip('/') + '/'
        return any(p.startswith(prefix) for p in (self._files | self._dirs))

    def is_file(self, path: str | Path) -> bool:
        return _norm(path) in self._files

    def is_dir(self, path: str | Path) -> bool:
        path = _norm(path)
        return path in self._dirs or self._has_descendant(path)

    def exists(self, path: str | Path) -> bool:
        return self.is_file(path) or self.is_dir(path)

    def add_file(self, path: str | Path) -> None:
        self._files.add(_norm(path))

    def add_dir(self, path: str | Path) -> None:
        self._dirs.add(_norm(path))


class SubprocessStepRunner:
    """Production step back-end: run the command as a real subprocess.

    Consolidates the Windows command resolution that used to live inline in
    `StepApi.__call__`, so the API layer no longer touches the OS directly.
    """

    def run(self, step: dict, *, check: bool,
            capture_output: bool) -> subprocess.CompletedProcess:
        import os  # Local imports: only the prod path needs these.
        import platform
        import shutil

        cmd = [str(arg) for arg in step['cmd']]
        if platform.system() == 'Windows':
            # Resolve to an absolute path to avoid bat-file name mismatches
            # (e.g. `gclient` vs `gclient.bat`) without using `shell=True`.
            resolved = shutil.which(cmd[0])
            if resolved is None:
                raise RuntimeError(f'Command not found: {cmd[0]}')
            cmd[0] = resolved

        # Compose the child environment from the context module's overrides and
        # path affixes over the real environment; when nothing was set, inherit
        # the parent environment unchanged (env=None).
        overrides = step.get('env')
        prefixes = step.get('env_prefixes')
        suffixes = step.get('env_suffixes')
        env = None
        if overrides or prefixes or suffixes:
            env = merge_envs(os.environ, overrides or {}, prefixes or {},
                             suffixes or {}, os.pathsep)
        return subprocess.run(cmd,
                              cwd=step.get('cwd'),
                              env=env,
                              check=check,
                              capture_output=capture_output,
                              text=True)


class SimulationStepRunner:
    """Simulation step back-end: record the step, return canned data.

    No subprocess ever runs. Each `run` appends a step dict to `recorded_steps`
    (the ordered stream the expectation is built from) and returns a
    `CompletedProcess` seeded from the step's `StepTestData`. When the step is
    `check`ed and its seeded retcode is non-zero, it raises the genuine
    `subprocess.CalledProcessError` so module `except CalledProcessError` arms
    and the recipe's failure path behave exactly as in production.
    """

    def __init__(self,
                 step_data: dict[str, StepTestData] | None = None) -> None:
        self._step_data = step_data or {}
        self.recorded_steps: list[dict] = []

    def run(self, step: dict, *, check: bool,
            capture_output: bool) -> subprocess.CompletedProcess:
        data = self._step_data.get(step['name'], StepTestData())
        cmd = [str(arg) for arg in step['cmd']]

        record: dict[str, Any] = {'name': step['name'], 'cmd': cmd}
        if step.get('cwd'):
            record['cwd'] = str(step['cwd'])
        if step.get('env'):
            # A `None` value means "remove this variable"; preserve it (rather
            # than stringifying to 'None') so it renders as null.
            record['env'] = {
                k: (None if v is None else str(v))
                for k, v in step['env'].items()
            }
        for affix in ('env_prefixes', 'env_suffixes'):
            if step.get(affix):
                record[affix] = {
                    k: [str(v) for v in values]
                    for k, values in step[affix].items()
                }
        if data.retcode:
            record['retcode'] = data.retcode
        self.recorded_steps.append(record)

        # Mirror subprocess: captured output is only available when the caller
        # asked to capture it; otherwise the child inherited the parent streams
        # and `stdout`/`stderr` come back as None.
        stdout = data.stdout if capture_output else None
        stderr = data.stderr if capture_output else None

        if check and data.retcode != 0:
            raise subprocess.CalledProcessError(data.retcode,
                                                cmd,
                                                output=stdout,
                                                stderr=stderr)
        return subprocess.CompletedProcess(cmd,
                                           data.retcode,
                                           stdout=stdout,
                                           stderr=stderr)


class TestContext:
    """Run-global simulated state, seeded onto every module in test mode."""

    def __init__(self,
                 *,
                 platform: str = 'linux',
                 env: dict[str, str] | None = None,
                 files: Iterable[str | Path] = (),
                 dirs: Iterable[str | Path] = (),
                 which_map: dict[str, str] | None = None,
                 home: str | Path = SIM_HOME,
                 step_data: dict[str, StepTestData] | None = None) -> None:
        self.platform = platform
        self.env = dict(env or {})
        self.fs = SimFS(files, dirs)
        self.which_map = dict(which_map or {})
        self.home = Path(str(home))
        self.step_runner = SimulationStepRunner(step_data)

    @classmethod
    def from_test_data(cls, test_data: TestData) -> TestContext:
        """Build a context from the seed values a `GenTests` case supplied."""
        mod = test_data.mod_data
        platform = mod.get('platform', {}).get('name', 'linux')
        env_seed = mod.get('env', {})
        path_seed = mod.get('path', {})
        return cls(
            platform=platform,
            # `api.env.set(...)` vars plus any `api.properties.environ(...)`
            # values; the latter is what the engine decodes into ENV_PROPERTIES.
            env={
                **env_seed.get('vars', {}),
                **test_data.environ
            },
            which_map=dict(env_seed.get('which', {})),
            files=[_resolve_seed(p) for p in path_seed.get('files', [])],
            dirs=[_resolve_seed(p) for p in path_seed.get('dirs', [])],
            step_data=test_data.step_data,
        )


def _resolve_seed(path: str) -> str:
    """Resolve a seeded path: absolute as-is, relative under the workspace."""
    pure = PurePosixPath(path)
    return str(pure if pure.is_absolute() else SIM_WORKSPACE / pure)


def stabilize(value: str, context: TestContext | None = None) -> str:
    """Rewrite machine-specific path prefixes to stable tokens."""
    value = value.replace(str(SIM_WORKSPACE), WORKSPACE_TOKEN)
    home = str(context.home) if context is not None else str(SIM_HOME)
    return value.replace(home, HOME_TOKEN)


def build_steps(runner: SimulationStepRunner, failure: dict | None,
                context: TestContext) -> dict[str, dict]:
    """Assemble the ordered `{name: step}` map (+ `$result`) for post-process.

    Paths are stabilized here so post-process checks and the written expectation
    see the same tokenized commands. `failure` is `None` for a successful run,
    otherwise it is the terminal result's `failure` payload. A non-infra failure
    carries an inner `{'failure': {}}`, which an infra failure does not. Any
    machine paths in its `humanReason` are stabilized too.
    """
    steps: dict[str, dict] = {}
    for step in runner.recorded_steps:
        entry: dict[str, Any] = {
            'name': step['name'],
            'cmd': [stabilize(arg, context) for arg in step['cmd']],
        }
        if 'cwd' in step:
            entry['cwd'] = stabilize(step['cwd'], context)
        if 'env' in step:
            entry['env'] = {
                k: (None if v is None else stabilize(v, context))
                for k, v in step['env'].items()
            }
        for affix in ('env_prefixes', 'env_suffixes'):
            if affix in step:
                entry[affix] = {
                    k: [stabilize(v, context) for v in values]
                    for k, values in step[affix].items()
                }
        if 'retcode' in step:
            entry['retcode'] = step['retcode']
        steps[step['name']] = entry

    result: dict[str, Any] = {'name': pp.RESULT_STEP}
    if failure is not None:
        if 'humanReason' in failure:
            failure = {
                **failure,
                'humanReason': stabilize(failure['humanReason'], context),
            }
        result['failure'] = failure
    steps[pp.RESULT_STEP] = result
    return steps


def apply_post_process(
        hooks: list[PostprocessHookContext],
        steps: dict[str, dict]) -> tuple[dict[str, dict] | None, list[Check]]:
    """Run post-process hooks.

    Each hook gets its own `Checker` and a deep copy of the current steps (the
    checker MUST be a local so its stack walk can find the frames to blame). A
    `KeyError` from a hook (e.g. indexing a step that didn't run) is rendered as
    a failed check rather than aborting the case. A hook that returns a mapping
    filters the steps for later hooks and for the written expectation, but only
    after `VerifySubset` confirms it introduced no new keys / reordering /
    changed values, otherwise a `PostProcessError` is raised. An empty mapping
    drops the expectation (returns `None`).
    """
    failed_checks: list[Check] = []
    for hook in hooks:
        working = copy.deepcopy(steps)
        # Kept in a local named `check`: `Checker._call_impl` walks the stack for
        # the frame in which the checker is a local variable.
        check = Checker(hook, working)
        try:
            result = hook.func(check, working, *hook.args, **hook.kwargs)
        except KeyError:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            try:
                failed_checks.append(
                    Check.create(
                        '',
                        hook,
                        inspect.getinnerframes(exc_traceback)[1:],
                        False,
                        check._ignore_set,  # pylint: disable=protected-access
                        {
                            'raised exception': '%s: %s' %
                            (exc_type.__name__, exc_value)
                        },
                    ))
            finally:
                # avoid reference cycle as suggested by inspect docs.
                del exc_traceback
            continue
        failed_checks += check.failed_checks
        if result is not None:
            msg = VerifySubset(result, steps)
            if msg:
                raise PostProcessError('post process: steps' + msg)
            # Restore 'name' if a filter dropped it.
            for name, step in result.items():
                step['name'] = name
            steps = result
    # An empty mapping means drop the expectation.
    return (steps if steps else None), failed_checks
