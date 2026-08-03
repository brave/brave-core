# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test doubles for `terminal.py`.

Two fixtures live here, both aimed at driving a tools/cr command end to end
against a `FakeChromiumRepo`:

* `FakeTerminal` replaces `terminal.run` with a router. Every `git` invocation
  is passed through to the real implementation, so all repository work in a
  test is genuine git against the fake checkout. The commands a fake checkout
  cannot possibly serve -- the `npm`/`pnpm` build commands, `gh`, and the
  `vpython3` helper scripts -- are emulated over the same fake repository, and
  anything unrecognised raises, so a command growing a new dependency cannot
  silently reach the network or the developer's machine.

* `ConsoleCapture` swaps the rich console every tools/cr module prints through
  for one writing to a buffer, which is how tests assert on what the user is
  shown.
"""

from __future__ import annotations

from functools import partial
from pathlib import Path
import io
import json
import subprocess
import sys

from unittest.mock import patch

from rich.console import Console

import terminal

from test.fake_chromium_repo import FakeChromiumRepo
from test.fake_gh import FakeGh

# Emulated commands, used to key the injected failures in
# `FakeTerminal.fail`. The `npm run <name>` ones are named after the script
# they run.
INIT = 'init'
APPLY_PATCHES = 'apply_patches'
UPDATE_PATCHES = 'update_patches'
CHROMIUM_REBASE_L10N = 'chromium_rebase_l10n'
PLASTER = 'plaster'
GNRT = 'gnrt'

# The stderr line `npm run init` ends with when a patch fails to apply, and the
# stdout header that precedes the JSON breakdown of the failures. Both are
# matched by brockit, so they are reproduced verbatim from
# `build/commands/lib/util.js` and `build/commands/lib/gitPatcherLog.ts`.
PATCH_FAILURE_STDERR_LINE = 'Exiting as not all patches were successful!'
PATCH_FAILURE_JSON_HEADER = '{count} Failed patches json breakdown:'


class FakeTerminal:
    """Routes `terminal.run`: real git, emulated everything else.

    Install it with `install()`, and read `calls` afterwards to assert on what
    the command under test ran.
    """

    def __init__(self,
                 repo: FakeChromiumRepo,
                 *,
                 gh: FakeGh | None = None) -> None:
        self._repo = repo

        # The `gh` CLI double every `gh` invocation is answered by.
        self.gh = gh or FakeGh()

        # Every command routed, in order, as a list of string arguments.
        self.calls: list[list[str]] = []

        # Injected failures, keyed by emulated command (see `fail`).
        self._failures: dict[str, str] = {}

        # Extra stderr prepended to the `npm run init` failure output. Used to
        # reproduce warnings the real command mixes into its own failures.
        self.init_extra_stderr: str = ''

        self._patcher = None
        self._real_run = None

    # -- installation -------------------------------------------------------

    def install(self) -> FakeTerminal:
        """Routes `terminal.run` through this object.

        The patch is applied to the `Terminal` singleton rather than to the
        class, which keeps the real implementation reachable for the commands
        that are passed through.
        """
        if self._patcher is not None:
            raise AssertionError('FakeTerminal is already installed.')
        self._real_run = partial(terminal.Terminal.run, terminal.terminal)
        self._patcher = patch.object(terminal.terminal, 'run', new=self)
        self._patcher.start()
        return self

    def uninstall(self) -> None:
        """Restores the real `terminal.run`."""
        if self._patcher is None:
            return
        self._patcher.stop()
        self._patcher = None
        self._real_run = None

    # -- failure injection --------------------------------------------------

    def fail(self, command: str, stderr: str = 'fake failure') -> None:
        """Makes an emulated command fail from now on.

        Args:
            command: One of the emulated command constants in this module.
            stderr: The stderr the failure carries.
        """
        self._failures[command] = stderr

    def _check_failure(self, command: str, cmd: list[str]) -> None:
        """Raises the injected failure for `command`, when there is one."""
        stderr = self._failures.get(command)
        if stderr is not None:
            raise subprocess.CalledProcessError(1,
                                                cmd,
                                                output='',
                                                stderr=stderr)

    # -- inspection ---------------------------------------------------------

    def npm_runs(self) -> list[str]:
        """The name of every `npm run <name>` routed, in order."""
        return [
            cmd[2] for cmd in self.calls
            if len(cmd) > 2 and Path(cmd[0]).stem in (
                'npm', 'pnpm') and cmd[1] == 'run'
        ]

    def ran(self, *prefix: str) -> bool:
        """Whether any routed command starts with `prefix`."""
        return any(cmd[:len(prefix)] == list(prefix) for cmd in self.calls)

    # -- routing ------------------------------------------------------------

    def __call__(self, cmd, **kwargs) -> subprocess.CompletedProcess:
        cmd = [str(x) for x in cmd]
        self.calls.append(cmd)

        program = Path(cmd[0]).stem
        if program == 'git':
            return self._run_git(cmd, **kwargs)
        if program in ('npm', 'pnpm'):
            return self._run_npm(cmd, **kwargs)
        if program == 'gh':
            return self.gh(cmd, **kwargs)
        if program.startswith('vpython3'):
            return self._run_vpython(cmd, **kwargs)
        raise AssertionError(
            f'FakeTerminal has no route for this command: {cmd}')

    @staticmethod
    def _completed(cmd: list[str],
                   stdout: str = '') -> subprocess.CompletedProcess:
        """A successful result shaped like the one `terminal.run` returns."""
        return subprocess.CompletedProcess(cmd, 0, stdout=stdout, stderr='')

    def _run_git(self, cmd: list[str],
                 **kwargs) -> subprocess.CompletedProcess:
        """Passes a git command through to the real implementation.

        Anything addressing a remote over the network is rejected instead: a
        test reaching out to Googlesource (or anywhere else) is a fixture that
        is missing a tag or a branch, not something to wait on.
        """
        remote = next((arg for arg in cmd if '://' in arg), None)
        if remote is not None:
            raise AssertionError(
                f'A test tried to reach {remote} with: {" ".join(cmd)}. Set '
                'the fake repository up so the command resolves locally.')
        return self._real_run(cmd, **kwargs)

    def _run_npm(self, cmd: list[str],
                 **kwargs) -> subprocess.CompletedProcess:
        """Emulates the `npm run <name>` build commands over the fake repo."""
        del kwargs  # The build commands are always run from the brave root.
        if cmd[1] != 'run':
            raise AssertionError(f'Unexpected package manager call: {cmd}')
        name = cmd[2]
        if name == INIT:
            return self._run_init(cmd)
        if name == APPLY_PATCHES:
            return self._run_apply_patches(cmd)
        if name == UPDATE_PATCHES:
            self._check_failure(UPDATE_PATCHES, cmd)
            self._repo.run_update_patches()
            return self._completed(cmd, 'Done.\n')
        if name == CHROMIUM_REBASE_L10N:
            self._check_failure(CHROMIUM_REBASE_L10N, cmd)
            files = self._repo.run_chromium_rebase_l10n()
            return self._completed(cmd, '\n'.join(files))
        raise AssertionError(f'FakeTerminal cannot emulate `npm run {name}`.')

    def _run_init(self, cmd: list[str]) -> subprocess.CompletedProcess:
        """Emulates `npm run init`: sync the checkout, then apply patches.

        A failure to apply is reported the way the real command reports it: the
        JSON breakdown is left out (only `apply_patches` is asked for it) and
        the last stderr line is the one brockit matches on to tell a patch
        failure from any other kind of `init` failure.
        """
        self._check_failure(INIT, cmd)
        self._repo.sync_chromium()
        failures = self._repo.run_apply_patches()
        if not failures:
            return self._completed(cmd, 'apply patches - done\n')
        raise subprocess.CalledProcessError(
            1,
            cmd,
            output=f'{len(failures)} patches failed to apply.\n',
            stderr=f'{self.init_extra_stderr}{PATCH_FAILURE_STDERR_LINE}')

    def _run_apply_patches(self,
                           cmd: list[str]) -> subprocess.CompletedProcess:
        """Emulates `npm run apply_patches`.

        With `--print-patch-failures-in-json` the failures are printed to
        stdout as the JSON breakdown brockit parses out of the failed run.
        """
        self._check_failure(APPLY_PATCHES, cmd)
        failures = self._repo.run_apply_patches()
        if not failures:
            return self._completed(cmd, 'apply patches - done\n')

        stdout = ''
        if '--print-patch-failures-in-json' in cmd:
            stdout = '%s\n%s\n' % (PATCH_FAILURE_JSON_HEADER.format(
                count=len(failures)), json.dumps(failures))
        raise subprocess.CalledProcessError(1,
                                            cmd,
                                            output=stdout,
                                            stderr=PATCH_FAILURE_STDERR_LINE)

    def _run_vpython(self, cmd: list[str],
                     **kwargs) -> subprocess.CompletedProcess:
        """Emulates the `vpython3` helper scripts a command shells out to."""
        del kwargs  # The scripts below are unaffected by cwd or env.
        script = Path(cmd[1]).name
        if script == 'plaster.py':
            self._check_failure(PLASTER, cmd)
            return self._completed(cmd, 'plaster check - done\n')
        if script == 'run_gnrt.py':
            self._check_failure(GNRT, cmd)
            files = self._repo.run_gnrt(cmd[2])
            return self._completed(cmd, '\n'.join(files))
        raise AssertionError(f'FakeTerminal cannot emulate script: {cmd}')


class ConsoleCapture:
    """Captures everything printed through the tools/cr rich console.

    tools/cr modules import the console by value (`from terminal import
    console`), so installing this fixture rebinds the name in every module that
    holds a reference to it. Timestamps and source locations are turned off, and
    the console is given a generous width, so what a test asserts on is the
    message itself rather than the log decorations around it.
    """

    # Wide enough that no message brockit logs wraps, which keeps assertions
    # about a line's content and indentation meaningful.
    DEFAULT_WIDTH = 200

    def __init__(self, width: int = DEFAULT_WIDTH) -> None:
        # The console writes into a buffer of its own rather than through a
        # redirected stdout: rich resolves `Console.file` lazily, so a console
        # with no file of its own writes to whatever `sys.stdout` is at print
        # time, and unittest's own output would end up in the capture.
        self._buffer = io.StringIO()
        self._console = Console(file=self._buffer,
                                width=width,
                                log_time=False,
                                log_path=False,
                                force_terminal=False,
                                no_color=True,
                                highlight=False)
        self._patchers: list = []

    def install(self) -> ConsoleCapture:
        """Rebinds `console` in every module that imported it."""
        if self._patchers:
            raise AssertionError('ConsoleCapture is already installed.')
        # Captured before patching anything: `terminal` is itself one of the
        # modules holding the reference being replaced.
        original = terminal.console
        for module in [m for m in list(sys.modules.values()) if m is not None]:
            if getattr(module, 'console', None) is original:
                patcher = patch.object(module, 'console', self._console)
                patcher.start()
                self._patchers.append(patcher)
        return self

    def uninstall(self) -> None:
        """Restores the real console everywhere it was rebound."""
        for patcher in self._patchers:
            patcher.stop()
        self._patchers = []

    def mark(self) -> int:
        """A marker for the current end of the capture, for use with `since`."""
        return len(self._buffer.getvalue())

    def since(self, mark: int) -> str:
        """Everything captured after `mark`, one line per printed line.

        Rich pads every logged line out to the console width; the padding is
        stripped here so assertions can be written against the text as it
        reads.
        """
        return '\n'.join(
            line.rstrip()
            for line in self._buffer.getvalue()[mark:].splitlines())

    @property
    def text(self) -> str:
        """Everything captured so far."""
        return self.since(0)
