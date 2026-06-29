# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The core `step` module API."""

from __future__ import annotations

from collections.abc import Mapping, Sequence
import logging
from pathlib import Path
import platform
import shutil
import subprocess

from recipe_api import RecipeApi


class StepApi(RecipeApi):
    """Runs a subprocess as a named build step.

    The instance is callable, so recipes and modules invoke it as
    `api.step(name, cmd, ...)`, mirroring `recipe_engine/step`. This is the one
    place that actually shells out; everything else describes work in terms of
    steps. Consolidates the logging and Windows command resolution that
    `build_rust_toolchain.py::_check_call` did inline.
    """

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
            cwd: Optional working directory for the subprocess.
            env: Optional environment mapping; inherits the parent when `None`.
            check: Raise `CalledProcessError` on non-zero exit when True.
            capture_output: Capture stdout/stderr (as text) instead of
                inheriting the parent's streams.

        Returns:
            The `subprocess.CompletedProcess` for the invocation.

        Raises:
            subprocess.CalledProcessError: If `check` and the process fails.
            RuntimeError: On Windows, if the command cannot be resolved.
        """
        cmd = [str(arg) for arg in cmd]
        logging.info('[step] %s\n >>>> %s', name, ' '.join(cmd))

        if platform.system() == 'Windows':
            # Resolve to an absolute path to avoid bat-file name mismatches
            # (e.g. `gclient` vs `gclient.bat`) without using `shell=True`.
            resolved = shutil.which(cmd[0])
            if resolved is None:
                raise RuntimeError(f'Command not found: {cmd[0]}')
            cmd[0] = resolved

        return subprocess.run(cmd,
                              cwd=cwd,
                              env=env,
                              check=check,
                              capture_output=capture_output,
                              text=True)
