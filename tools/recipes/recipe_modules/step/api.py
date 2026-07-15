# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The core `step` module API."""

from __future__ import annotations

from collections.abc import Mapping, Sequence
import logging
from pathlib import Path
import subprocess

from recipe_api import RecipeApi


class StepApi(RecipeApi):
    """Runs a subprocess as a named build step.

    The instance is callable, so recipes and modules invoke it as
    `api.step(name, cmd, ...)`, mirroring `recipe_engine/step`. This is the one
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
    """

    def __init__(self) -> None:
        super().__init__()
        # Lazily-created production runner (unused in test mode, where the
        # engine seeds `self._test.step_runner`).
        self._prod_runner = None

    def _runner(self):
        if self._test is not None:
            return self._test.step_runner
        if self._prod_runner is None:
            # Imported lazily so `step` stays dependency-free at import time and
            # simulation code isn't loaded on the production path until needed.
            from simulation import SubprocessStepRunner
            self._prod_runner = SubprocessStepRunner()
        return self._prod_runner

    def __call__(
            self,
            name: str,
            cmd: Sequence[str | Path],
            *,
            cwd: str | Path | None = None,
            env: Mapping[str, str] | None = None,
            check: bool = True,
            capture_output: bool = False) -> subprocess.CompletedProcess[str]:
        """Run *cmd* as the step named *name*.

        Args:
            name: Human-readable step name, logged before the command runs.
            cmd: The program and its arguments as a sequence; each element is
                stringified (so `Path` objects are fine).
            cwd: Working directory override for the subprocess; defaults to
                `context.cwd` (and, when neither is set, the engine's cwd).
            env: Whole-variable environment overrides layered on top of
                `context.env` (the explicit value wins per key).
            check: Raise `CalledProcessError` on non-zero exit when True.
            capture_output: Capture stdout/stderr (as text) instead of
                inheriting the parent's streams.

        Returns:
            The `subprocess.CompletedProcess` for the invocation.

        Raises:
            subprocess.CalledProcessError: If `check` and the process fails.
            RuntimeError: On Windows, if the command cannot be resolved.
        """
        # Draw cwd/env from the ambient context, letting explicit arguments
        # override. The env overrides and the path prefix/suffix affixes are
        # carried separately so the runner can compose them (production) or
        # record them (simulation), mirroring recipe_engine's split.
        context = self.m.context
        env_overrides = {**context.env, **(env or {})}
        step = {
            'name': name,
            'cmd': [str(arg) for arg in cmd],
            'cwd': cwd if cwd is not None else context.cwd,
            'env': env_overrides or None,
            'env_prefixes': context.env_prefixes,
            'env_suffixes': context.env_suffixes,
        }
        logging.info('[step] %s\n >>>> %s', name, ' '.join(step['cmd']))
        return self._runner().run(step,
                                  check=check,
                                  capture_output=capture_output)
