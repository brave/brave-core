# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The core `step` module API."""

from __future__ import annotations

from collections.abc import Callable, Mapping, Sequence
import contextlib
import logging
from pathlib import Path
import subprocess

import config_types
from engine_types import ResourceCost as _ResourceCost
from recipe_api import (InputPlaceholder, OutputPlaceholder, Placeholder,
                        RecipeApi)
from recipe_test_api import PlaceholderTestData, StepTestData
from resource_semaphore import ResourceWaiter
from step_data import StepData

# _UNSET_COST means "the caller said nothing about cost", and the default,
# which is also different from an explicit `cost=None`, which opts the step out
# of admission control entirely.
_UNSET_COST = object()


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
        # Admission control for steps that declare a `cost`, sized from the
        # host in initialise().
        self._resource: ResourceWaiter | None = None

    def initialise(self) -> None:
        self._resource = ResourceWaiter(
            self.m.platform.cpu_count * self.CPU_CORE,
            self.m.platform.total_memory)

    def ResourceCost(self,
                     cpu: int = 500,
                     memory: int = 50,
                     disk: int = 0,
                     net: int = 0) -> _ResourceCost:
        """A structure defining the resources that a given step may need.

        The four resources are:

          * cpu (measured in millicores): The amount of cpu the step is expected
            to take. Defaults to 500.
          * memory (measured in MiB): The amount of memory the step is expected
            to take. Defaults to 50.
          * disk (as percentage of max disk bandwidth): The amount of "disk
            bandwidth" the step is expected to take. This is a very simplified
            percentage covering IOPS, read/write bandwidth, seek time, etc. At
            100, the step will run exclusively w.r.t. all other steps having a
            `disk` cost. At 0, the step will run regardless of other steps with
            disk cost.
          * net (as percentage of max net bandwidth): The amount of "net
            bandwidth" the step is expected to take. As `disk`, but for the
            network.

        A step will run when ALL of the resources are simultaneously available.
        The engine uses a greedy scheduling algorithm for picking the next step
        to run: if several are waiting, it picks the largest which fits the
        currently available resources.

        A value higher than the machine's maximum is equivalent to that maximum,
        so a step declaring more memory than the host has still runs -- on its
        own.

        Returns:
            A `ResourceCost` suitable for `api.step(...)`'s `cost` kwarg.
            Passing `None` as the cost is equivalent to
            `ResourceCost(0, 0, 0, 0)`.
        """
        return _ResourceCost(min(cpu, self.MAX_CPU),
                             min(memory, self.MAX_MEMORY), disk, net)

    # The number of millicores in a single CPU core.
    CPU_CORE = 1000

    @property
    def MAX_CPU(self) -> int:
        """The maximum number of millicores this system has."""
        return self.m.platform.cpu_count * self.CPU_CORE

    @property
    def MAX_MEMORY(self) -> int:
        """The maximum amount of memory on the system, in MiB."""
        return self.m.platform.total_memory

    @property
    def active_result(self) -> StepData | None:
        """The currently active (open) result from the last step that was run.

        Allows you to do things like:

            try:
                api.step('run test', [..., api.json.output()])
            finally:
                result = api.step.active_result
                if result.json.output:
                    ...

        `None` before this greenlet has run any step.
        """
        return self._step_stack.active_step

    @contextlib.contextmanager
    def nest(self, name: str):
        """Nest the steps run inside the `with` block under a parent step.

        This runs a step of its own, with no command, and names every step run
        within it under it:

            with api.step.nest('build'):
                api.step('configure', ['gn', 'gen'])   # `build.configure`
                api.step('compile', ['ninja'])         # `build.compile`

        Nesting composes, so a nest inside a nest extends the namespace
        further. Because names are only made distinct within their namespace,
        two nests may each contain a step of the same name.

        A nest also owns the work spawned inside it: leaving the block waits
        for any greenlet `futures.spawn` started there, so

            with api.step.nest('fan out'):
                for url in urls:
                    api.futures.spawn(fetch, url)

        cannot fall out of the block with fetches still in flight.

        Args:
            name: The name of this step.

        Yields:
            The parent step's `StepData`.
        """
        assert name, 'invalid empty name'

        parent = self(name, None)
        # The step just run is the tip, recorded under the right namespace.
        # Promoting it to a parent is what puts the steps inside the block
        # under it, rather than beside it, and keeps it open until the block
        # ends.
        self._step_stack.make_tip_parent()
        try:
            yield parent
        finally:
            # Close whatever leaf the block left open, then the parent itself,
            # which waits for the greenlets spawned inside it.
            self._step_stack.close_non_parent_step()
            self._step_stack.pop()

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
        cmd: Sequence[str | Path | config_types.Path | Placeholder] | None,
        *,
        cwd: str | Path | None = None,
        env: Mapping[str, str] | None = None,
        check: bool = True,
        stdin: InputPlaceholder | None = None,
        stdout: OutputPlaceholder | None = None,
        stderr: OutputPlaceholder | None = None,
        cost: _ResourceCost | None = _UNSET_COST,
        step_test_data: Callable[[], StepTestData] | None = None,
    ) -> StepData:
        """Run *cmd* as the step named *name*.

        Args:
            name: Human-readable step name, logged before the command runs. It
                is namespaced by any enclosing `nest`, and made distinct if the
                same name has already been used in that namespace.
            cmd: The program and its arguments as a sequence; each element is
                either a `Placeholder` (rendered into arguments just before the
                step runs) or stringified (so `Path` objects are fine). `None`
                runs no subprocess, recording a step that exists only to say
                something happened.
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
            cost: What the step is expected to consume, from
                `api.step.ResourceCost(...)`. The step waits until the machine
                has room for all of it at once, so several expensive steps
                cannot overwhelm the host. Defaults to `ResourceCost()` (half a
                core, 50 MiB) for a step with a command, and to nothing for a
                command-less one. Pass `None` to opt out of admission control.
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
        # Namespaces the step under any enclosing `nest` and makes a repeated
        # name distinct. Also closes the previous leaf step, which stayed open
        # so `active_result` could reach it.
        name_tokens = self._step_stack.record_step_name(name)
        name = '.'.join(name_tokens)

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
        for arg in cmd or ():
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

        def _if_blocking(needed: _ResourceCost) -> None:
            # Only reached when a step actually queues, which a simulated run
            # never does: a simulated step completes without yielding, so two
            # of them never hold resources at once. `ResourceWaiter` itself is
            # covered by `unittests/resource_semaphore_test.py`, which runs
            # greenlets that really do block.
            logging.info(  # pragma: no cover
                '[step] %s waiting for resources: %s', name, needed)

        # A command-less step runs nothing, so it consumes nothing and must
        # never queue; anything else takes the default cost unless the caller
        # said otherwise.
        if cost is _UNSET_COST:
            cost = None if not rendered_cmd else self.ResourceCost()
        with self._resource.wait_for(cost, _if_blocking):
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
        # Pushed before the failure is raised, so a recipe can still read the
        # failing step's result from `active_result` in a `finally`.
        self._step_stack.push(result, name_tokens)

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
