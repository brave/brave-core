# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from contextlib import contextmanager
import logging
import platform
import secrets
import shutil
import subprocess
import sys
import threading
import time

from rich.console import Console

# The interval in second for the keep-alive ping on infra mode to be printed.
KEEP_ALIVE_PING_INTERVAL = 20

KEEP_ALIVE_PING_ART = [
    '(-_-)', '(⊙_⊙)', '(¬_¬)', '(－‸ლ)', '(◎_◎;)', '(⌐■_■)', '(•‿•)', '(≖_≖)'
]

# The rich console used for all terminal output. Defined here (rather than
# at the end of the file) so that the import-time logging preset below can
# route through it.
console = Console()


def is_verbose() -> bool:
    """Returns True if `--verbose` was passed on the command line.

    Reads `sys.argv` directly so the answer is available at module import
    time, before any `argparse` parser has had a chance to run.
    """
    return '--verbose' in sys.argv


class _PresetLoggingHandler(logging.Handler):
    """Baseline logging handler used both at import time and as the base
    class for `IncendiaryErrorHandler`.

    Renders DEBUG records in dim styling via `console.log`; all other
    levels are emitted as plain rich-console log lines.
    """

    # Stack offset passed to `console.log` so the file:line column points
    # at the caller's `logging.<level>(...)` site instead of into Python's
    # logging internals. Subclasses that add their own `emit` frame on top
    # of this one must override this with `_STACK_OFFSET + 1`.
    _STACK_OFFSET = 8

    def emit(self, record: logging.LogRecord) -> None:
        msg = record.getMessage()
        if record.levelno == logging.DEBUG:
            console.log(f'[dim]{msg}[/]', _stack_offset=self._STACK_OFFSET)
        else:
            console.log(msg, _stack_offset=self._STACK_OFFSET)


class IncendiaryErrorHandler(_PresetLoggingHandler):
    """Logging handler used by tools/cr entry-point `main()` functions.

    Inherits the dim-DEBUG / plain-other-levels behavior from
    `_PresetLoggingHandler` and adds a loud emoji prefix to ERROR records
    so failures stand out in the terminal output.
    """

    # One extra frame on top of `_PresetLoggingHandler.emit` (this class's
    # `emit` calls `super().emit(record)`).
    _STACK_OFFSET = _PresetLoggingHandler._STACK_OFFSET + 1

    def emit(self, record: logging.LogRecord) -> None:
        if record.levelno == logging.ERROR:
            record.msg = f'¯\\_(ツ)_/¯\n🔥🔥 {record.msg}'
        super().emit(record)


# Baseline logging config installed at import time. Without this, debug
# logs emitted during module import (e.g. `_compute_brave_core_path` in
# repository.py) would be dropped because entry-point `main()` functions
# only call `logging.basicConfig` *after* their imports finish. Entry
# points can still override this with `logging.basicConfig(..., force=True)`
# to install custom handlers/formatting.
logging.basicConfig(level=logging.DEBUG if is_verbose() else logging.INFO,
                    handlers=[_PresetLoggingHandler()])


class Terminal:
    """A class that holds the application data and methods.
    """

    def __init__(self):
        # The status object to update with the terminal.
        self.status = None

        # The inital part of the status message, used as a prefix for all
        # updates.
        self.starting_status_message = ''

        # flag indicating if the terminal is running on infra.
        self.infra_mode = False

        # The time when a commmand run was started to check for keep alive
        # pings. Only relevant when running on infra.
        self.current_command_start_time = None

        # The keep-alive thread for terminal pings on infra mode.
        self.keep_alive_thread = threading.Thread(
            target=self.keep_alive_ci_feedback, daemon=True)

        # The command that is currently running on infra mode.
        self.running_command = None

    def keep_alive_ci_feedback(self):
        """Main routine for the keep-alive ping on infra mode.

    This routine runs on a separate thread for the entirety of the  run when on
    infra mode, sleeping for a set ping interval.

    The routine effects are only visible when `current_command_start_time` is
    set, at which point there's a chance a ping will be printed to the console
    in the set time for the ping interval.

    Leaving the back thread running is the best way to ensure we can avoid
    joining the thread, and all sorts of complexities relating to sleeping and
    joining, which could result in a minimum time every command has to take.
        """
        while True:
            if self.current_command_start_time:
                elapsed_time = time.time() - self.current_command_start_time
                if elapsed_time > KEEP_ALIVE_PING_INTERVAL:
                    logging.info('%s\n        [dim]>>>> %s[/]',
                                 secrets.choice(KEEP_ALIVE_PING_ART),
                                 self.running_command)
            time.sleep(KEEP_ALIVE_PING_INTERVAL)

    def set_infra_mode(self):
        """Sets the terminal to run on infra.
        """
        self.infra_mode = True
        self.keep_alive_thread.start()

        # let's give it some large width to avoid wrapping in jenkins
        console.size = console.size._replace(width=200)

    def _set_status_object(self, status):
        """Preserves the status object for updates.

    This function is used to preserve the status object for updates, so that
    the status can be updated with the initial status message.
    """
        self.status = status
        self.starting_status_message = status.status

    def update_status(self, status_message: str):
        if self.infra_mode or not self.status:
            return

        self.status.update(f'{self.starting_status_message} '
                           f'[bold cyan]({status_message})[/]')

    @contextmanager
    def with_status(self, status_message: str):
        """Context manager to manage `rich.console.status` internally.

        Does nothing when `infra_mode` is enabled.

        Args:
            status_message: The message to display in the console status.
        """
        if self.infra_mode:
            yield  # No-op when in infra mode
            return

        status = console.status(f'[bold green]{status_message}[/]')
        status.start()
        self._set_status_object(status)
        try:
            yield status
        finally:
            status.stop()
            self.status = None

    def run(self,
            cmd,
            *,
            env: dict[str, str] | None = None,
            cwd=None,
            interactive: bool = False,
            stdin: str | None = None):
        """Runs a command on the terminal.

        When `interactive=True`, the subprocess inherits the parent's
        stdin/stdout/stderr instead of capturing them -- use this for
        spawning editors, pagers, or anything else that needs to take
        over the tty. The returned `CompletedProcess.stdout` /
        `.stderr` are `None` in that case (nothing is captured).

        `env` follows the same semantics as `subprocess.run`: `None` inherits
        the parent's environment, a dict fully replaces it. If you want to
        add a few keys on top of `os.environ`, copy it first
        (`env={**os.environ, ...}`).

                Pass `stdin=` to feed data into the subprocess's stdin. Captured
        (non-interactive) mode encodes it with utf-8 to match the
        captured streams; interactive mode rejects `stdin=` because the
        subprocess owns the tty.
        """
        # Convert all arguments to strings, to avoid issues with `PurePath`
        # being passed arguments
        cmd = [str(x) for x in cmd]

        def truncate_on_max_length(message):
            # Truncate at the first newline if it exists
            if '\n' in message:
                message = message.split('\n')[0] + '...'
            # Truncate at max_length if the message is still too long
            max_length = console.size.width - len(
                self.starting_status_message) - 10
            if len(message) > max_length:
                message = message[:max_length - 3] + '...'
            return message

        if not self.infra_mode:
            self.update_status(truncate_on_max_length(" ".join(cmd)))
        logging.debug('λ %s', ' '.join(cmd))

        # We don't want the keep alive messages to be printed when interactive
        # is True, as we are delegating to the called command to provide its own
        # feedback.
        arm_keep_alive = self.infra_mode and not interactive
        if arm_keep_alive:
            self.current_command_start_time = time.time()
            self.running_command = " ".join(cmd)

        if platform.system() == 'Windows':
            # On Windows, resolve the command to an absolute path to avoid
            # issues with bat/cmd wrappers (e.g. `npm` → `npm.cmd`). This
            # avoids the use of shell=True.
            resolved = shutil.which(cmd[0])
            if resolved is None:
                raise RuntimeError(f'Command not found: {cmd[0]}')
            if resolved != cmd[0]:
                cmd = [resolved] + cmd[1:]

        # Captured mode pairs `capture_output` with text decoding so the
        # `.stdout` / `.stderr` strings on the result are usable directly.
        # Interactive mode skips both -- stdio is inherited from the parent,
        # nothing is captured, and `text` / `encoding` are irrelevant.
        capture_kwargs: dict[str, object] = {}
        if not interactive:
            capture_kwargs.update(capture_output=True,
                                  text=True,
                                  encoding='utf-8')

        if interactive and stdin is not None:
            raise ValueError(
                'terminal.run(): `stdin=` is not supported with '
                '`interactive=True` (the subprocess owns the tty).')

        # The status spinner has to be stopped before running any commands in
        # interactive mode, to not interfere with that process's output.
        paused_status = self.status if interactive and self.status else None
        if paused_status is not None:
            paused_status.stop()

        try:
            result = subprocess.run(cmd,
                                    check=True,
                                    env=env,
                                    cwd=cwd,
                                    input=stdin,
                                    **capture_kwargs)
        except subprocess.CalledProcessError as e:
            if e.stderr:
                # Only captured in non-interactive mode.
                logging.debug('❯ %s', e.stderr.strip())
            raise e
        finally:
            if paused_status is not None:
                paused_status.start()
            if arm_keep_alive:
                self.current_command_start_time = None
                self.running_command = None

        return result

    def run_git(self,
                *cmd,
                no_trim=False,
                env: dict[str, str] | None = None) -> str:
        """Runs a git command with the arguments provided.

    This function returns a proper utf8 string in success, otherwise it allows
    the exception thrown by subprocess through.

    Args:
        *cmd: The command to run, with any arguments.
        no_trim: If True, the output will not be trimmed. This is usually rare
        but preferred when producing the contents of a file.
        env: Optional environment variables to be forwarded to `terminal.run`.
    e.g:
        self.run_git('add', '-u', '*.patch')
    """
        cmd = ['git'] + list(cmd)
        if no_trim:
            return self.run(cmd, env=env).stdout
        return self.run(cmd, env=env).stdout.strip()

    def log_task(self, message):
        """Logs a task to the console using common decorators
        """
        console.log(f'[bold red]*[/] {message}')

    def run_npm_command(self, *cmd):
        """Runs an npm build command.

      This function will run 'npm run' commands appended by any extra arguments
      are passed into it.

      e.g:
          self.run_npm_command('init')
      """
        cmd = ['npm', 'run'] + list(cmd)
        if self.infra_mode and len(cmd) == 3 and cmd[-1] == 'init':
            # Special flag to avoid running into issues in jenkins when running
            # `gclient sync` with `--revision`. For more details see:
            # https://github.com/brave/brave-browser/issues/44921
            cmd += ['--', '--with_issue_44921']
        return self.run(cmd)


terminal = Terminal()
