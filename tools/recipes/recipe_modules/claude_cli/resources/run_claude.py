#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Run `claude --output-format stream-json` and print a readable digest of it.

The raw stream is kept in full in the `--log` file; the console gets Claude's
messages, one line per tool call, the tail of each tool's output, and the
final result. The output of commands Claude runs in the background is followed
live, until Claude is notified that they finished, and so are the `*.log`
files of the `--logs` directory, which commands write to instead of Claude's
context:

    run_claude.py --log .autolift/cr159/claude.jsonl --logs .autolift/cr159 \
        -- claude --print ...
"""

from __future__ import annotations

import argparse
import collections
import contextlib
import ctypes
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import threading
import time
from typing import Any, Callable

# How many lines of a tool's output are printed, and how many characters go
# on one console line before a line goes on over the next.
TAIL_LINES = 15
LINE_WIDTH = 160

# How often a background command's output file is read, in seconds.
POLL_INTERVAL = 1.0

# The tool inputs that best summarise a call, by order of preference.
_SUMMARY_KEYS = (
    'description',
    'file_path',
    'pattern',
    'command',
    'url',
    'query',
    'prompt',
)

# The tool result of a command started in the background.
_BACKGROUND_RE = re.compile(
    r'Command running in background with ID: (?P<id>\w+)\. '
    r'Output is being written to: (?P<path>\S+?)\.?(?:\s|$)'
)

# What each line of a tool call is: its command, its output (or an error's),
# and what is streamed from logs and background commands.
_COMMAND = '  ┃ $ '
# Later lines of a command line up with the command after the `$`.
_COMMAND_GOES_ON = '  ┃   '
_OUTPUT = '  ┆ '
# An error's output alternates them, down a zig-zag line.
_ERROR = ('  ╲ ', '  ╱ ')
_STREAMED = '  ┊ '

_RESET = '\x1b[0m'

# Brockit's log lines end in the right-aligned source of the log call.
_LOG_SOURCE_RE = re.compile(r'\s{2,}[\w.]+\.py:\d+$')

# How long the processes left behind get to exit after SIGTERM, in seconds,
# before they are killed. Long enough for a build to finish writing: siso
# killed partway left empty entries in its shared local cache, which later
# builds then failed on (cr159: 1,733 blobs).
KILL_GRACE = 120.0

# How often adopted processes that exited are collected, in seconds.
ORPHAN_POLL_INTERVAL = 0.2

# From `<linux/prctl.h>`.
_PR_SET_PDEATHSIG = 1
_PR_SET_CHILD_SUBREAPER = 36


def _wrap(prefix: str, text: str, continuation: str | None = None) -> list[str]:
    """*text* after *prefix*, over as many console lines as it takes, the
    later ones after *continuation* (*prefix* if not given)."""
    chunks = [
        text[i : i + LINE_WIDTH] for i in range(0, len(text), LINE_WIDTH)
    ] or ['']
    rest = prefix if continuation is None else continuation
    return [prefix + chunks[0], *(rest + chunk for chunk in chunks[1:])]


def _clean(line: str) -> str:
    """A line as a terminal would end up showing it, without log sources.

    Progress output rewrites its line with `\\r`, so only what follows the
    last `\\r` is kept.
    """
    line = line.rsplit('\r', 1)[-1].rstrip()
    return _LOG_SOURCE_RE.sub('', line)


def _tool_summary(name: str, tool_input: dict[str, Any]) -> list[str]:
    """The lines for a tool call: its name and its most telling input, and
    the command it runs, if that isn't the input already shown."""
    lines = [f'▶ {name}']
    for key in _SUMMARY_KEYS:
        value = tool_input.get(key)
        if isinstance(value, str) and value.strip():
            first = value.strip().splitlines()[0]
            lines = _wrap('▶ ', f'{name}: {first}', '  ')
            break
    command = tool_input.get('command')
    if isinstance(command, str) and tool_input.get('description'):
        # In full, as what a command does is often in its later lines.
        first, *rest = command.strip().splitlines()
        lines += _wrap(_COMMAND, first, _COMMAND_GOES_ON)
        for line in rest:
            lines += _wrap(_COMMAND_GOES_ON, line)
    return lines


def _result_text(content: Any) -> str:
    """The text of a tool result, which is a string or a list of blocks."""
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        return '\n'.join(
            block.get('text', '')
            for block in content
            if isinstance(block, dict) and block.get('type') == 'text'
        )
    return ''


def _tail(text: str) -> list[str]:
    """The last non-empty lines of *text*, cleaned."""
    # Not `splitlines()`, which also splits on `\r`.
    lines = [_clean(line) for line in text.split('\n')]
    return [line for line in lines if line][-TAIL_LINES:]


def _duration(milliseconds: int) -> str:
    minutes, seconds = divmod(round(milliseconds / 1000), 60)
    hours, minutes = divmod(minutes, 60)
    return f'{hours}h{minutes:02d}m' if hours else f'{minutes}m{seconds:02d}s'


def background_commands(event: dict[str, Any]) -> list[tuple[str, Path]]:
    """The `(task id, output file)` of each command *event* reports started in
    the background."""
    if event.get('type') != 'user':
        return []
    found = []
    for block in (event.get('message') or {}).get('content') or []:
        if isinstance(block, dict) and block.get('type') == 'tool_result':
            match = _BACKGROUND_RE.search(_result_text(block.get('content')))
            if match:
                found.append((match['id'], Path(match['path'])))
    return found


def format_event(event: dict[str, Any]) -> list[str]:
    """The console lines for one event of the stream; none for most."""
    kind = event.get('type')
    subtype = event.get('subtype')
    message = event.get('message') or {}
    # Events of subagents name the tool call that started them.
    indent = '    ' if event.get('parent_tool_use_id') else ''
    lines: list[str] = []

    if kind == 'system' and subtype == 'init':
        lines.append(
            f'● Session {event.get("session_id")} ({event.get("model")})'
        )
    elif kind == 'system' and subtype == 'task_notification':
        lines.append(f'  ⏹ {event.get("summary") or event.get("status")}')
    elif kind == 'assistant':
        for block in message.get('content') or []:
            if block.get('type') == 'text' and block.get('text', '').strip():
                lines += block['text'].strip().splitlines()
            elif block.get('type') == 'tool_use':
                # An empty line sets each call apart from what came before.
                lines.append('')
                lines += _tool_summary(
                    block.get('name', '?'), block.get('input') or {}
                )
    elif kind == 'user':
        for block in message.get('content') or []:
            if not isinstance(block, dict):
                continue
            if block.get('type') != 'tool_result':
                continue
            texts = [
                text
                for line in _tail(_result_text(block.get('content')))
                for text in _wrap('', line)
            ]
            if block.get('is_error'):
                lines += [_ERROR[i % 2] + text for i, text in enumerate(texts)]
            else:
                lines += [_OUTPUT + text for text in texts]
    elif kind == 'result':
        parts = [f'■ {subtype}']
        if event.get('duration_ms'):
            parts.append(f'after {_duration(event["duration_ms"])}')
        if event.get('num_turns'):
            parts.append(f'{event["num_turns"]} turns')
        if isinstance(event.get('total_cost_usd'), (int, float)):
            parts.append(f'${event["total_cost_usd"]:.2f}')
        # Not `result`, which repeats Claude's last message.
        lines.append(', '.join(parts))

    return [f'{indent}{line}' if line else '' for line in lines]


def parse_line(line: str) -> dict[str, Any] | None:
    """The event on one line of the stream, or None for any other line."""
    try:
        event = json.loads(line)
    except ValueError:
        return None
    return event if isinstance(event, dict) else None


class Follower:
    """Prints the lines written to a file, as they are written."""

    def __init__(self, path: Path, emit: Callable[[str], None]) -> None:
        self._path = path
        self._emit = emit
        self._done = threading.Event()
        self._thread = threading.Thread(target=self._follow, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        """Prints what is left of the output, and stops following it."""
        self._done.set()
        self._thread.join()

    def _follow(self) -> None:
        offset = 0
        pending = ''
        while True:
            # Read once more after `stop`, for the last of the output.
            done = self._done.wait(POLL_INTERVAL)
            try:
                with self._path.open('rb') as file:
                    file.seek(offset)
                    data = file.read()
            except FileNotFoundError:
                data = b''
            offset += len(data)
            pending += data.decode('utf-8', errors='replace')
            *lines, pending = pending.split('\n')
            if done:
                lines.append(pending)
            for line in lines:
                if cleaned := _clean(line):
                    for text in _wrap(_STREAMED, cleaned):
                        self._emit(text)
            if done:
                return


class LogsFollower:
    """Prints the `*.log` files of a directory as they are written, each
    under a line naming it."""

    def __init__(self, directory: Path, emit: Callable[[str], None]) -> None:
        self._directory = directory
        self._emit = emit
        self._followers: list[Follower] = []
        self._done = threading.Event()
        self._thread = threading.Thread(target=self._watch, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        """Prints what is left of the logs, and stops following them."""
        self._done.set()
        self._thread.join()
        for follower in self._followers:
            follower.stop()

    def _watch(self) -> None:
        seen: set[Path] = set()
        while True:
            # Look once more after `stop`, for a log that just appeared.
            done = self._done.wait(POLL_INTERVAL)
            # The directory may not exist yet.
            for path in sorted(self._directory.glob('*.log')):
                if path not in seen:
                    seen.add(path)
                    self._emit(f'{_STREAMED}▸ {path.name}')
                    self._followers.append(Follower(path, self._emit))
            if done:
                return


class Digest:
    """Prints the digest of the stream, following background commands, and
    the logs in *logs_dir*, if given."""

    def __init__(
        self, emit: Callable[[str], None], logs_dir: Path | None = None
    ) -> None:
        self._lock = threading.Lock()
        self._print = emit
        self._followers: dict[str, Follower] = {}
        self._logs = LogsFollower(logs_dir, self.emit) if logs_dir else None
        self.failed = False
        # Background work stopped with no turn of Claude's after it: Claude
        # stopped what it was waiting on, rather than the work finishing.
        self.abandoned: list[str] = []

    def emit(self, text: str) -> None:
        if '\x1b[' in text:
            # Output may colour a line and leave the reset for a line the
            # digest leaves out, so each line ends with one.
            text += _RESET
        with self._lock:
            self._print(text)

    def handle_line(self, line: str) -> None:
        event = parse_line(line)
        if event is None:
            # Not an event, e.g. a warning printed before the stream.
            if line.strip():
                self.emit(line.rstrip())
            return
        if event.get('subtype') == 'task_notification':
            # The command's output is done before its notification.
            follower = self._followers.pop(str(event.get('task_id')), None)
            if follower is not None:
                follower.stop()
            if event.get('status') == 'stopped':
                self.abandoned.append(
                    str(event.get('summary') or event.get('task_id'))
                )
        if event.get('type') == 'assistant':
            self.abandoned.clear()
        if event.get('type') == 'result' and event.get('is_error'):
            self.failed = True
        for text in format_event(event):
            self.emit(text)
        for task_id, path in background_commands(event):
            self._followers[task_id] = Follower(path, self.emit)

    def close(self) -> None:
        for follower in self._followers.values():
            follower.stop()
        self._followers.clear()
        if self._logs is not None:
            self._logs.stop()
            self._logs = None


def _prctl(option: int, value: int) -> None:
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.prctl(option, value, 0, 0, 0) != 0:
        raise OSError(ctypes.get_errno(), f'prctl({option}) failed')


def _descendants(root: int) -> list[int]:
    """The PIDs of every process below *root*, from `/proc`."""
    children = collections.defaultdict(list)
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():
            continue
        try:
            stat = (entry / 'stat').read_text(encoding='utf-8')
        except OSError:
            continue
        # The command name, in parentheses, may itself hold spaces.
        ppid = int(stat.rsplit(')', 1)[1].split()[1])
        children[ppid].append(int(entry.name))
    found = []
    pending = [root]
    while pending:
        for child in children[pending.pop()]:
            found.append(child)
            pending.append(child)
    return found


def _reap() -> None:
    """Collects the exit status of the children that exited."""
    with contextlib.suppress(ChildProcessError):
        while os.waitpid(-1, os.WNOHANG)[0]:
            pass


class OrphanReaper:
    """Collects the exit status of the processes this one adopted, as they
    exit, but never that of *keep*, which its owner waits for.

    Left as zombies, their PIDs still look alive. Xvfb, for one, then takes
    the lock file of an earlier Xvfb it can't clean up for that of a running
    server, and refuses to start.
    """

    def __init__(self, keep: int) -> None:
        self._keep = keep
        self._done = threading.Event()
        self._thread = threading.Thread(target=self._reap, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._done.set()
        self._thread.join()

    def _reap(self) -> None:
        while not self._done.wait(ORPHAN_POLL_INTERVAL):
            while True:
                try:
                    # Looks without collecting, so that *keep* is left alone.
                    info = os.waitid(
                        os.P_ALL, 0, os.WEXITED | os.WNOHANG | os.WNOWAIT
                    )
                except ChildProcessError:
                    break
                if info is None or info.si_pid == self._keep:
                    break
                os.waitpid(info.si_pid, 0)


def kill_descendants() -> None:
    """Stops every process left below this one: SIGTERM, then SIGKILL for
    those still there after `KILL_GRACE`."""
    for sig in (signal.SIGTERM, signal.SIGKILL):
        pids = _descendants(os.getpid())
        if not pids:
            return
        for pid in pids:
            with contextlib.suppress(ProcessLookupError):
                os.kill(pid, sig)
        deadline = time.monotonic() + KILL_GRACE
        while time.monotonic() < deadline:
            _reap()
            if not _descendants(os.getpid()):
                return
            time.sleep(0.1)


def _exit_on_signal(signum: int, _frame: Any) -> None:
    raise SystemExit(128 + signum)


def run(cmd: list[str], log: Path, logs_dir: Path | None = None) -> int:
    """Runs *cmd*, logging its stream to *log* and printing the digest.

    Whatever *cmd* leaves running when it exits, or when this is stopped, is
    stopped too.

    Returns the exit code: Claude's own, or 1 if it reported an error result
    while exiting 0.
    """
    log.parent.mkdir(parents=True, exist_ok=True)
    digest = Digest(lambda text: print(text, flush=True), logs_dir)

    def pump(stream, log_file) -> None:
        for line in stream:
            log_file.write(line)
            log_file.flush()
            digest.handle_line(line)

    # Appended to, so that a resumed run keeps the stream of the one it
    # carries on.
    with (
        log.open('a', encoding='utf-8') as log_file,
        subprocess.Popen(
            cmd, stdout=subprocess.PIPE, text=True, encoding='utf-8'
        ) as process,
    ):
        # Read on the side: what Claude leaves running may hold its stdout
        # open, so the end of the stream can't tell that Claude is done.
        reader = threading.Thread(
            target=pump, args=(process.stdout, log_file), daemon=True
        )
        reader.start()
        reaper = OrphanReaper(process.pid)
        try:
            returncode = process.wait()
        finally:
            reaper.stop()
            kill_descendants()
            # The stream ends once nothing below holds it open any more.
            reader.join(timeout=KILL_GRACE)
            process.stdout.close()
            digest.close()
    if digest.abandoned:
        digest.emit(
            f'■ Failed: Claude stopped {len(digest.abandoned)} background '
            'tasks before they finished, and did not carry on after them:'
        )
        for task in digest.abandoned:
            digest.emit(f'  ⏹ {task}')
    return returncode or int(digest.failed or bool(digest.abandoned))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        '--log', type=Path, required=True, help='Where to keep the raw stream.'
    )
    parser.add_argument(
        '--logs', type=Path, help='A directory whose `*.log` files to follow.'
    )
    parser.add_argument('cmd', nargs='+', help='The claude command, after --.')
    args = parser.parse_args(argv)

    engine = os.getppid()
    # Claude runs background commands in sessions of their own, which signals
    # to its process group don't reach, and which outlive Claude. As a
    # subreaper, this process inherits them when their parent exits, so that
    # `run` can stop them.
    _prctl(_PR_SET_CHILD_SUBREAPER, 1)
    # The engine stopping, even by SIGKILL, stops this too, and so the rest.
    _prctl(_PR_SET_PDEATHSIG, signal.SIGTERM)
    for signum in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(signum, _exit_on_signal)
    if os.getppid() != engine:
        # The engine was gone before the death signal was armed.
        return 1
    return run(args.cmd, args.log, args.logs)


if __name__ == '__main__':
    sys.exit(main())
