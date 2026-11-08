# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `claude_cli` module API: run Claude Code headless, as a step.

The step runs `claude --print` through `resources/run_claude.py`, which keeps
Claude's whole `stream-json` output in a log and prints a readable digest of it
to the console, along with the `*.log` files the run writes as they grow.
"""

from __future__ import annotations

from collections.abc import Iterable
from pathlib import Path
from typing import TYPE_CHECKING

import config_types
from recipe_api import RecipeApi
from step_data import StepData

if TYPE_CHECKING:
    from recipe_modules import claude_cli

# Where `run` keeps Claude's whole stream, in the run's log directory.
STREAM_LOG = 'claude.jsonl'


class ClaudeCliApi(RecipeApi):
    """Runs Claude Code unattended, in auto permission mode."""

    m: claude_cli.DEPS

    def run(
        self,
        name: str,
        prompt: str,
        *,
        log_dir: Path | config_types.Path,
        add_dirs: Iterable[str | Path | config_types.Path] = (),
        allowed_tools: Iterable[str] = (),
        must_read: Iterable[str | Path | config_types.Path] = (),
        report: Path | config_types.Path | None = None,
    ) -> StepData:
        """Run Claude on *prompt* as the step named *name*, until it finishes.

        The step fails if Claude does, or ends with an error result. Claude
        keeps its own temporary files in the job's temp dir, and may write
        there too, so nothing it does lands in the system's temp dir.

        Args:
            name: The step name.
            prompt: What Claude is asked to do.
            log_dir: Where Claude's stream is kept (`claude.jsonl`), and whose
                `*.log` files are followed on the console as they are written.
            add_dirs: Directories Claude may work in, beyond its cwd.
            allowed_tools: Tool permission rules that skip auto mode's review,
                e.g. `Edit(/<dir>/**)`.
            must_read: Files the prompt asks Claude to read in full; those it
                didn't read all of itself are named at the end, and in
                *report*.
            report: Where to name them.

        Returns:
            The step's `StepData`.
        """
        cmd = [
            self.m.depot_tools.vpython3(),
            self.resource('run_claude.py'),
            '--log',
            log_dir / STREAM_LOG,
            '--logs',
            log_dir,
        ]
        for path in must_read:
            cmd += ['--must-read', path]
        if report is not None:
            cmd += ['--report', report]
        cmd += [
            '--',
            'claude',
            '--print',
            prompt,
            '--permission-mode',
            'auto',
            '--output-format',
            'stream-json',
            '--verbose',
        ]
        temp_dir = self.m.path.temp_dir
        for directory in [*add_dirs, temp_dir]:
            cmd += ['--add-dir', directory]
        cmd += [
            '--allowedTools',
            *allowed_tools,
            f'Edit(/{temp_dir}/**)',
            f'Write(/{temp_dir}/**)',
        ]
        env = {
            # Claude Code's own temporary files (background commands'
            # output, its sandbox's `TMPDIR`) go here rather than to the
            # system's temp dir, which `TMPDIR` alone doesn't change for it.
            'CLAUDE_CODE_TMPDIR': str(temp_dir),
            # `--print` otherwise stops subagents and background commands
            # still running 10 minutes after Claude's last turn, and reports
            # success.
            'CLAUDE_CODE_PRINT_BG_WAIT_CEILING_MS': '0',
            # Each Bash command starts in the step's cwd: a `cd` doesn't carry
            # over to commands that assume they run there.
            'CLAUDE_BASH_MAINTAIN_PROJECT_WORKING_DIR': '1',
        }
        with self.m.context(env=env):
            return self.m.step(name, cmd)
