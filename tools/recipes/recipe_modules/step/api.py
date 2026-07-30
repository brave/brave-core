# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The core `step` module API."""

from __future__ import annotations

from collections.abc import Callable, Mapping, Sequence
import logging
from pathlib import Path
import subprocess

from recipe_api import (InputPlaceholder, OutputPlaceholder, Placeholder,
                        RecipeApi)
from recipe_test_api import PlaceholderTestData, StepTestData
from step_data import StepData


class StepApi(RecipeApi):
    """Runs a subprocess as a named build step.

    The instance is callable, so recipes and modules invoke it as
    `api.step(name, cmd, ...)`. This is the one
    place work actually happens; everything else describes work in terms of
    steps. The API only *describes* a step (name + command + cwd/env) and hands
    it to a step runner: in production a `SubprocessStepRunner` shells out (with
    Windows command resolution); under test the engine seeds a
    `SimulationStepRunner` that records the step and returns canned data, so no
    subprocess runs.

    Each step's cwd and environment are drawn from the ambient `context` module
    (`with api.context(...)`): the step inherits `context.cwd` and applies
    `context.env` / `env_prefixes` / `env_suffixes`. Explicit `cwd` / `env`
    arguments, when given, layer on top of that ambient state.

    A step's command may contain placeholders (see `recipe_api.Placeholder`),
    and its `stdin`/`stdout`/`stderr` may each be one. This module renders them
    into real arguments before the step runs and collects their results into the
    `StepData` it returns afterwards.
    """

    def __init__(self) -> None:
        super().__init__()
        # Lazily-created production runner (unused in test mode, where the
        # engine seeds `self._test.step_runner`).
        self._prod_runner = None

    def _runner(self):
        if self._test is None:  # pragma: no cover - production step backend.
            return self._prod_runner_lazy()
        return self._test.step_runner

    def _prod_runner_lazy(self):  # pragma: no cover - production step backend.
        if self._prod_runner is None:
            # Imported lazily so `step` stays dependency-free at import time and
            # simulation code isn't loaded on the production path until needed.
            from simulation import SubprocessStepRunner
            self._prod_runner = SubprocessStepRunner()
        return self._prod_runner

    def __call__(
        self,
        name: str,
        cmd: Sequence[str | Path | Placeholder],
        *,
        cwd: str | Path | None = None,
        env: Mapping[str, str] | None = None,
        check: bool = True,
        stdin: InputPlaceholder | None = None,
        stdout: OutputPlaceholder | None = None,
        stderr: OutputPlaceholder | None = None,
        step_test_data: Callable[[], StepTestData] | None = None,
    ) -> StepData:
        """Run *cmd* as the step named *name*.

        Args:
            name: Human-readable step name, logged before the command runs.
            cmd: The program and its arguments as a sequence; each element is
                either a `Placeholder` (rendered into arguments just before the
                step runs) or stringified (so `Path` objects are fine).
            cwd: Working directory override for the subprocess; defaults to
                `context.cwd` (and, when neither is set, the engine's cwd).
            env: Whole-variable environment overrides layered on top of
                `context.env` (the explicit value wins per key).
            check: Raise `CalledProcessError` on non-zero exit when True.
            stdin: An input placeholder whose data is fed to the command, e.g.
                `api.json.input({'k': 'v'})`.
            stdout: An output placeholder capturing the command's standard
                output, e.g. `api.raw_io.output_text()`. Its result is the
                returned `StepData`'s `stdout`. Left unset, the command
                inherits this process's stdout.
            stderr: As *stdout*, for standard error.
            step_test_data: A zero-argument callable returning the
                `StepTestData` this step should default to under simulation, so
                the common case needs no per-test seeding, e.g.
                `lambda: api.raw_io.stream_output_text('hello\\n')`. A test can
                add to it with `api.step_data` or discard it with
                `api.override_step_data`.

        Returns:
            The `StepData` for the invocation: its retcode, the results of its
            `stdout`/`stderr` placeholders, and one attribute per output
            placeholder in *cmd*.

        Raises:
            subprocess.CalledProcessError: If `check` and the process fails.
            RuntimeError: On Windows, if the command cannot be resolved.
        """
        runner = self._runner()
        test_data = runner.step_test_data(name, step_test_data)

        # A placeholder's simulated data is looked up once and reused, so
        # rendering and reading a result back agree on what was seeded.
        placeholder_tests: dict[tuple[str, str, str | None],
                                PlaceholderTestData] = {}

        def _test_for(placeholder: Placeholder) -> PlaceholderTestData:
            key = (*placeholder.namespaces, placeholder.name)
            if key not in placeholder_tests:
                placeholder_tests[key] = test_data.pop_placeholder(*key)
            return placeholder_tests[key]

        rendered_cmd: list[str] = []
        cmd_placeholders: list[Placeholder] = []
        for arg in cmd:
            if isinstance(arg, Placeholder):
                rendered_cmd.extend(arg.render(_test_for(arg)))
                cmd_placeholders.append(arg)
            else:
                rendered_cmd.append(str(arg))

        handles = _render_handles(stdin, stdout, stderr, test_data)

        # Draw cwd/env from the ambient context, letting explicit arguments
        # override. The env overrides and the path prefix/suffix affixes are
        # carried separately so the runner can compose them (production) or
        # record them (simulation).
        context = self.m.context
        env_overrides = {**context.env, **(env or {})}
        step = {
            'name': name,
            'cmd': rendered_cmd,
            'cwd': cwd if cwd is not None else context.cwd,
            'stdin': handles['stdin'],
            'env': env_overrides or None,
            'env_prefixes': context.env_prefixes,
            'env_suffixes': context.env_suffixes,
        }
        logging.info('[step] %s\n >>>> %s', name, ' '.join(rendered_cmd))
        retcode = runner.run(step, handles)

        # Resolve placeholders before reporting a failure, so that a failing
        # step still cleans up its input files and still reports whatever
        # output it did produce.
        result = StepData(name, retcode)
        for placeholder in cmd_placeholders:
            placeholder_test = _test_for(placeholder)
            if isinstance(placeholder, InputPlaceholder):
                placeholder.cleanup(placeholder_test.enabled)
            else:
                result.assign_placeholder(placeholder,
                                          placeholder.result(placeholder_test))
        if stdin is not None:
            stdin.cleanup(test_data.stdin.enabled)
        if stdout is not None:
            result.stdout = stdout.result(test_data.stdout)
        if stderr is not None:
            result.stderr = stderr.result(test_data.stderr)
        result.finalize()

        if check and retcode != 0:
            raise subprocess.CalledProcessError(retcode,
                                                rendered_cmd,
                                                output=result.stdout,
                                                stderr=result.stderr)
        return result


def _render_handles(stdin: InputPlaceholder | None,
                    stdout: OutputPlaceholder | None,
                    stderr: OutputPlaceholder | None,
                    test_data) -> dict[str, str | None]:
    """Render the step's std handle placeholders to their backing files.

    A handle with no placeholder maps to `None`, meaning "inherit this
    process's".
    """
    handles: dict[str, str | None] = {}
    for handle, placeholder, expected in (('stdin', stdin, InputPlaceholder),
                                          ('stdout', stdout,
                                           OutputPlaceholder),
                                          ('stderr', stderr,
                                           OutputPlaceholder)):
        if placeholder is None:
            handles[handle] = None
            continue
        if not isinstance(placeholder, expected):
            raise ValueError(
                f"a step's {handle} must be an {expected.__name__}; got "
                f'{placeholder!r}')
        if not placeholder.is_file_backed:
            raise ValueError(
                f"a step's {handle} must be backed by a single file; "
                f'{placeholder!r} is not')
        placeholder.render(getattr(test_data, handle))
        handles[handle] = placeholder.backing_file
    return handles
