# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from contextlib import contextmanager
import logging
import platform
import secrets
import subprocess
import threading
import time
from typing import Optional, Dict

from rich.console import Console

# The interval in second for the keep-alive ping on infra mode to be printed.
KEEP_ALIVE_PING_INTERVAL = 20

KEEP_ALIVE_PING_ART = [
    '(-_-)', '(⊙_⊙)', '(¬_¬)', '(－‸ლ)', '(◎_◎;)', '(⌐■_■)', '(•‿•)', '(≖_≖)'
]


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

    def run(self, cmd, env: Optional[Dict[str, str]] = None, cwd=None):
        """Runs a command on the terminal.
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

        if self.infra_mode:
            self.current_command_start_time = time.time()
            self.running_command = " ".join(cmd)

        try:
            # It is necessary to pass `shell=True` on Windows, otherwise the
            # process handle is entirely orphan and can't resolve things like
            # `npm`.
            result = subprocess.run(cmd,
                                    capture_output=True,
                                    text=True,
                                    check=True,
                                    env=env,
                                    cwd=cwd,
                                    shell=platform.system() == 'Windows')
        except subprocess.CalledProcessError as e:
            logging.debug('❯ %s', e.stderr.strip())
            raise e
        finally:
            if self.infra_mode:
                self.current_command_start_time = None
                self.running_command = None

        return result

    def run_git(self, *cmd, no_trim=False) -> str:
        """Runs a git command with the arguments provided.

    This function returns a proper utf8 string in success, otherwise it allows
    the exception thrown by subprocess through.

    Args:
        *cmd: The command to run, with any arguments.
        no_trim: If True, the output will not be trimmed. This is usually rare
        but preferred when producing the contents of a file.
    e.g:
        self.run_git('add', '-u', '*.patch')
    """
        cmd = ['git'] + list(cmd)
        if no_trim:
            return self.run(cmd).stdout
        return self.run(cmd).stdout.strip()

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


console = Console()
terminal = Terminal()
