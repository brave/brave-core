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
    live in `StepApi`) and redirects the step's std handles at the files its
    placeholders rendered to; simulation records an ordered step list, runs
    nothing, and hands back the canned retcode. Both also answer the `step`
    module's request for a step's simulated data -- production with a
    `DisabledTestData`, so placeholders know they are running for real.
  * expectation helpers: `stabilize` (the one remaining real machine path,
    `RECIPES_ROOT`, -> `[RECIPES_ROOT]`), `build_steps`, and
    `apply_post_process`. `[WORKSPACE]`/`[HOME]` need no such rewriting: the
    `path` recipe module builds them as literal `config_types.Path` tokens
    from the start (see `recipe_modules/path/api.py`), so they never appear as
    real machine paths in the first place.

Deliberately never imports `engine` (the engine imports this), so there is no
cycle: the test runner in `engine.py` drives a recipe, then hands the recorded
steps here for post-processing and serialization.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Callable, Iterable
import copy
import inspect
from pathlib import Path, PurePosixPath
import sys
from typing import Any

# gevent's drop-in `subprocess`, so a running step yields to other greenlets
# rather than blocking the single OS thread they all share. Without this the
# `futures` module would hand out concurrency that never actually overlaps.
from gevent import subprocess

import config_types
from check import Check, Checker, PostProcessError, VerifySubset
from engine_env import merge_envs
import post_process as pp
from recipe_test_api import (DisabledTestData, PostprocessHookContext,
                             StepTestData, TestData)

# The literal tokens `recipe_modules/path/api.py` builds its simulated
# `workspace`/`home` config_types.Path values from directly -- there is no
# real-looking fake path to later rewrite into these; the token is the value
# from construction (see config_types.py's module docstring).
WORKSPACE_TOKEN = '[WORKSPACE]'
HOME_TOKEN = '[HOME]'

# What a simulated host reports about itself, matching upstream recipes-py's
# simulator: linux/64/intel on 8 cores with 16 GiB. Fixed so a step's
# `ResourceCost` schedules the same way regardless of the machine running the
# tests; a case that needs different figures asks for them explicitly with
# `api.platform(...)` / `api.platform.capacity(...)`.
SIM_BITS = 64
SIM_ARCH = 'intel'
SIM_CPU_COUNT = 8
SIM_TOTAL_MEMORY = 16 * 1024

# This engine's own root (`tools/recipes/`), for stabilizing commands that
# reference a resource file living next to a recipe module (e.g. `file`'s
# `resources/fileutil.py`) via a real `Path(__file__).resolve()`-derived
# path -- unlike `WORKSPACE_TOKEN`/`HOME_TOKEN`, this is a real machine path
# even in test mode, so it varies by checkout location unless stabilized here.
# Computed independently of (but always equal to) `engine.RECIPES_ROOT`,
# since both are `Path(__file__).resolve().parent` of a sibling file in this
# same directory; this module deliberately never imports `engine` (see the
# module docstring).
RECIPES_ROOT = Path(__file__).resolve().parent
RECIPES_ROOT_TOKEN = '[RECIPES_ROOT]'


def _norm(path: str | Path | config_types.Path) -> str:
    """Normalize a path to a stable string key for the simulated filesystem.

    `config_types.Path.as_posix()` is used (not `str()`) because `str()`
    renders with whatever separator the current case is simulating -- under a
    `platform='win'` case that's `\\`, and `PurePosixPath` would then treat
    the whole backslash-joined string as one opaque filename component.
    """
    if isinstance(path, config_types.Path):
        return path.as_posix()
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

    def copy(self, source: str | Path, dest: str | Path) -> None:
        """Duplicates `source`, and everything nested under it, to `dest`."""
        source = _norm(source)
        dest = _norm(dest)
        if source == dest:
            raise ValueError(f'source and dest are the same path: {source!r}')
        prefix = source.rstrip('/') + '/'
        for collection in (self._files, self._dirs):
            for path in [
                    p for p in collection
                    if p == source or p.startswith(prefix)
            ]:
                collection.add(dest + path[len(source):])

    def remove(self,
               path: str | Path,
               should_remove: Callable[[str], bool] = lambda p: True) -> None:
        """Removes `path`, and everything nested under it matching
        `should_remove`, from the simulated filesystem."""
        path = _norm(path)
        prefix = path.rstrip('/') + '/'
        for collection in (self._files, self._dirs):
            collection -= {
                p
                for p in collection
                if (p == path or p.startswith(prefix)) and should_remove(p)
            }


class SubprocessStepRunner:
    """Production step back-end: run the command as a real subprocess.

    Consolidates the Windows command resolution that used to live inline in
    `StepApi.__call__`, so the API layer no longer touches the OS directly.
    """

    def step_test_data(
        self, name: str, step_test_data_fn: Callable[[], StepTestData] | None
    ) -> DisabledTestData:
        """Nothing is simulated here: every lookup answers "not simulated"."""
        del name, step_test_data_fn
        return DisabledTestData()

    def run(
        self, step: dict, handles: dict[str, str | None]
    ) -> int:  # pragma: no cover - production step backend.
        import contextlib  # Local imports: only the prod path needs these.
        import os
        import platform
        import shutil

        cmd = [str(arg) for arg in step['cmd']]
        if not cmd:
            # A command-less step (`api.step(name, None)`, and so every nest
            # step) records that something happened without running anything.
            return 0
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

        # Point the child's std handles at the files the step's placeholders
        # rendered to. A handle with no placeholder is left inherited, so a
        # step's output still reaches the build log by default.
        with contextlib.ExitStack() as stack:

            def _open(handle: str, mode: str):
                path = handles.get(handle)
                if path is None:
                    return None
                return stack.enter_context(open(path, mode))

            return subprocess.run(cmd,
                                  cwd=step.get('cwd'),
                                  env=env,
                                  check=False,
                                  stdin=_open('stdin', 'rb'),
                                  stdout=_open('stdout', 'wb'),
                                  stderr=_open('stderr', 'wb')).returncode


class SimulationStepRunner:
    """Simulation step back-end: record the step, return the canned retcode.

    No subprocess ever runs. Each `run` appends a step dict to `recorded_steps`
    (the ordered stream the expectation is built from). The step's simulated
    data -- its retcode and its placeholders' contents -- is handed to the
    `step` module by `step_test_data`, which merges what the test case seeded
    for the step on top of the step's own `step_test_data=` default.
    """

    def __init__(self, test_data: TestData | None = None) -> None:
        self._test_data = test_data if test_data is not None else TestData()
        # step name -> the data handed out for it, so `run` can read back the
        # retcode the case seeded without the `step` module passing it along.
        self._used_steps: dict[str, StepTestData] = {}
        self.recorded_steps: list[dict] = []

    def step_test_data(
            self, name: str, step_test_data_fn: Callable[[], StepTestData]
        | None) -> StepTestData:
        """The simulated data for the step named *name*."""
        data = self._test_data.get_step_test_data(
            name, step_test_data_fn or StepTestData)
        self._used_steps[name] = data
        return data

    def run(self, step: dict, handles: dict[str, str | None]) -> int:
        # The std handles' backing files are of no interest to the expectation
        # (they are temporary, and their contents come from the test data),
        # except for stdin: what a step is fed is worth recording.
        del handles
        record: dict[str, Any] = {
            'name': step['name'],
            'cmd': [str(arg) for arg in step['cmd']],
        }
        if step.get('cwd'):
            record['cwd'] = str(step['cwd'])
        if step.get('stdin') is not None:
            record['stdin'] = str(step['stdin'])
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
        retcode = self._used_steps[step['name']].retcode
        if retcode:
            record['retcode'] = retcode
        self.recorded_steps.append(record)
        return retcode


class TestContext:
    """Run-global simulated state, seeded onto every module in test mode."""

    def __init__(self,
                 *,
                 platform: str = 'linux',
                 bits: int = SIM_BITS,
                 arch: str = SIM_ARCH,
                 cpu_count: int = SIM_CPU_COUNT,
                 total_memory: int = SIM_TOTAL_MEMORY,
                 env: dict[str, str] | None = None,
                 files: Iterable[str | Path] = (),
                 dirs: Iterable[str | Path] = (),
                 which_map: dict[str, str] | None = None,
                 home: str = HOME_TOKEN,
                 test_data: TestData | None = None) -> None:
        self.platform = platform
        self.bits = bits
        self.arch = arch
        self.cpu_count = cpu_count
        self.total_memory = total_memory
        self.env = dict(env or {})
        self.fs = SimFS(files, dirs)
        self.which_map = dict(which_map or {})
        # The resolved-base string `_SimFs.home()` wraps in a fresh
        # `config_types.Path` -- a literal token by default (`[HOME]`).
        self.home = str(home)
        # Per-prefix counter behind `api.path.mkdtemp`, so a run's temporary
        # directories get stable, ordered names instead of random ones.
        self.temp_counter: Counter[str] = Counter()
        self.step_runner = SimulationStepRunner(test_data)

    @classmethod
    def from_test_data(cls, test_data: TestData) -> TestContext:
        """Build a context from the seed values a `GenTests` case supplied."""
        mod = test_data.mod_data
        platform_seed = mod.get('platform', {})
        platform = platform_seed.get('name', 'linux')
        env_seed = mod.get('env', {})
        path_seed = mod.get('path', {})
        return cls(
            platform=platform,
            bits=platform_seed.get('bits', SIM_BITS),
            arch=platform_seed.get('arch', SIM_ARCH),
            cpu_count=platform_seed.get('cpu_count', SIM_CPU_COUNT),
            total_memory=platform_seed.get('total_memory', SIM_TOTAL_MEMORY),
            # `api.env.set(...)` vars plus any `api.properties.environ(...)`
            # values; the latter is what the engine decodes into ENV_PROPERTIES.
            env={
                **env_seed.get('vars', {}),
                **test_data.environ
            },
            which_map=dict(env_seed.get('which', {})),
            files=[_resolve_seed(p) for p in path_seed.get('files', [])],
            dirs=[_resolve_seed(p) for p in path_seed.get('dirs', [])],
            test_data=test_data,
        )


def _resolve_seed(path: str) -> str:
    """Resolve a seeded path: absolute or already-tokenized (a leading
    `[TOKEN]`, e.g. `[HOME]/...`) as-is, relative under the workspace."""
    pure = PurePosixPath(path)
    if pure.is_absolute() or path.startswith('['):
        return str(pure)
    return f'{WORKSPACE_TOKEN}/{pure}'


def stabilize(value: str) -> str:
    """Rewrite the one remaining real machine path prefix to a stable token.

    `[WORKSPACE]`/`[HOME]` need no rewriting here: `recipe_modules/path/api.py`
    builds them as literal `config_types.Path` tokens from construction, so
    they never appear as real machine paths in a step's recorded strings.
    """
    return value.replace(str(RECIPES_ROOT), RECIPES_ROOT_TOKEN)


def build_steps(runner: SimulationStepRunner,
                failure: dict | None) -> dict[str, dict]:
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
            'cmd': [stabilize(arg) for arg in step['cmd']],
        }
        if 'cwd' in step:
            entry['cwd'] = stabilize(step['cwd'])
        if 'stdin' in step:
            entry['stdin'] = stabilize(step['stdin'])
        if 'env' in step:
            entry['env'] = {
                k: (None if v is None else stabilize(v))
                for k, v in step['env'].items()
            }
        for affix in ('env_prefixes', 'env_suffixes'):
            if affix in step:
                entry[affix] = {
                    k: [stabilize(v) for v in values]
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
                'humanReason': stabilize(failure['humanReason']),
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
