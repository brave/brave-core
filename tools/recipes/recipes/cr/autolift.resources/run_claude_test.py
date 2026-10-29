#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for run_claude.py."""

import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest

import run_claude

# `run_claude.py` runs on Linux only: it stops processes through `prctl` and
# `/proc`, and expects files written with `\n` line ends.
_linux_only = unittest.skipUnless(sys.platform == 'linux', 'Linux only')


def _assistant(*content, parent=None):
    return {
        'type': 'assistant',
        'message': {'content': list(content)},
        'parent_tool_use_id': parent,
    }


def _tool_result(text, is_error=False):
    return {
        'type': 'user',
        'message': {
            'content': [
                {'type': 'tool_result', 'content': text, 'is_error': is_error}
            ]
        },
    }


class FormatEventTest(unittest.TestCase):
    def test_init(self):
        self.assertEqual(
            run_claude.format_event(
                {
                    'type': 'system',
                    'subtype': 'init',
                    'session_id': 's1',
                    'model': 'opus',
                }
            ),
            ['● Session s1 (opus)'],
        )

    def test_text_and_tool_use(self):
        event = _assistant(
            {'type': 'thinking', 'thinking': 'hidden'},
            {'type': 'text', 'text': 'Rebase finished.\nChecking.'},
            {
                'type': 'tool_use',
                'name': 'Bash',
                'input': {'command': 'git status', 'description': 'Status'},
            },
            {'type': 'tool_use', 'name': 'Read', 'input': {'file_path': '/a'}},
            {'type': 'tool_use', 'name': 'TodoWrite', 'input': {}},
        )
        self.assertEqual(
            run_claude.format_event(event),
            [
                'Rebase finished.',
                'Checking.',
                '',
                '▶ Bash: Status',
                '  ┃ $ git status',
                '',
                '▶ Read: /a',
                '',
                '▶ TodoWrite',
            ],
        )

    def test_commands_are_shown_in_full(self):
        width = run_claude.LINE_WIDTH
        command = 'a' * width + 'b\ngit diff --stat'
        event = _assistant(
            {
                'type': 'tool_use',
                'name': 'Bash',
                'input': {'command': command, 'description': 'Check'},
            }
        )
        self.assertEqual(
            run_claude.format_event(event),
            [
                '',
                '▶ Bash: Check',
                f'  ┃ $ {"a" * width}',
                '  ┃   b',
                '  ┃   git diff --stat',
            ],
        )

    def test_command_without_description_is_shown_once(self):
        event = _assistant(
            {'type': 'tool_use', 'name': 'Bash', 'input': {'command': 'ls'}}
        )
        self.assertEqual(run_claude.format_event(event), ['', '▶ Bash: ls'])

    def test_log_sources_are_stripped(self):
        text = '[12:02:25] * Changes for this bump        terminal.py:358   '
        self.assertEqual(
            run_claude.format_event(_tool_result(text)),
            ['  ┆ [12:02:25] * Changes for this bump'],
        )

    def test_task_notification(self):
        event = {
            'type': 'system',
            'subtype': 'task_notification',
            'task_id': 'b1',
            'status': 'completed',
            'summary': 'Background command "Build" completed (exit code 0)',
        }
        self.assertEqual(
            run_claude.format_event(event),
            ['  ⏹ Background command "Build" completed (exit code 0)'],
        )

    def test_subagent_is_indented(self):
        event = _assistant({'type': 'text', 'text': 'hi'}, parent='toolu_1')
        self.assertEqual(run_claude.format_event(event), ['    hi'])

    def test_tool_result_tail_collapses_progress(self):
        lines = [str(i) for i in range(run_claude.TAIL_LINES)]
        text = '\n'.join(['Rebasing (1/3)\rRebasing (3/3)', '', *lines])
        # The progress line is the one too many.
        self.assertEqual(
            run_claude.format_event(_tool_result(text)),
            [f'  ┆ {line}' for line in lines],
        )
        self.assertEqual(
            run_claude.format_event(_tool_result('a\rb\nc')),
            ['  ┆ b', '  ┆ c'],
        )

    def test_tool_result_blocks_and_errors(self):
        event = _tool_result([{'type': 'text', 'text': 'boom'}], True)
        self.assertEqual(run_claude.format_event(event), ['  ╲ boom'])

    def test_error_output_zig_zags(self):
        width = run_claude.LINE_WIDTH
        event = _tool_result('x' * width + 'y\nfatal: no branch', True)
        self.assertEqual(
            run_claude.format_event(event),
            [f'  ╲ {"x" * width}', '  ╱ y', '  ╲ fatal: no branch'],
        )

    def test_long_lines_go_on_over_more_lines(self):
        width = run_claude.LINE_WIDTH
        text = 'a' * width + 'b' * width + 'c'
        self.assertEqual(
            run_claude.format_event(_tool_result(text)),
            [f'  ┆ {"a" * width}', f'  ┆ {"b" * width}', '  ┆ c'],
        )

    def test_result(self):
        event = {
            'type': 'result',
            'subtype': 'success',
            'duration_ms': 3_725_000,
            'num_turns': 12,
            'total_cost_usd': 1.5,
            'result': 'Lift done.',
        }
        self.assertEqual(
            run_claude.format_event(event),
            ['■ success, after 1h02m, 12 turns, $1.50'],
        )

    def test_other_events_print_nothing(self):
        self.assertEqual(
            run_claude.format_event(
                {'type': 'system', 'subtype': 'thinking_tokens'}
            ),
            [],
        )


def _started_in_background(task_id, path):
    return _tool_result(
        f'Command running in background with ID: {task_id}. Output is being '
        f'written to: {path}. You will be notified when it completes.'
    )


def _notification(task_id):
    return {
        'type': 'system',
        'subtype': 'task_notification',
        'task_id': task_id,
        'summary': f'{task_id} completed',
    }


@_linux_only
class BackgroundTest(unittest.TestCase):
    def setUp(self):
        self._poll_interval = run_claude.POLL_INTERVAL
        run_claude.POLL_INTERVAL = 0.01
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)
        self.output = self.tmp / 'b1.output'
        self.started = _started_in_background('b1', self.output)
        # The lines printed for the start, which wraps if the path is long.
        self.started_lines = len(run_claude.format_event(self.started))

    def tearDown(self):
        run_claude.POLL_INTERVAL = self._poll_interval

    def test_background_commands(self):
        self.assertEqual(
            run_claude.background_commands(
                _started_in_background('b1', '/tmp/tasks/b1.output')
            ),
            [('b1', Path('/tmp/tasks/b1.output'))],
        )
        self.assertEqual(
            run_claude.background_commands(_tool_result('done')), []
        )

    def test_output_is_followed_until_notified(self):
        printed = []
        digest = run_claude.Digest(printed.append)
        digest.handle_line(json.dumps(self.started))
        # Written after the command started, the last line unterminated.
        self.output.write_text(
            '[1/2] CXX a.o\n'
            'step 1\rstep 2\n'
            'error: boom        terminal.py:12\n'
            'FAILED',
            encoding='utf-8',
        )
        digest.handle_line(json.dumps(_notification('b1')))
        digest.close()
        self.assertEqual(
            printed[self.started_lines :],
            [
                '  ┊ [1/2] CXX a.o',
                '  ┊ step 2',
                '  ┊ error: boom',
                '  ┊ FAILED',
                '  ⏹ b1 completed',
            ],
        )

    def test_followers_stop_when_the_run_ends(self):
        printed = []
        digest = run_claude.Digest(printed.append)
        digest.handle_line(json.dumps(self.started))
        self.output.write_text('still building\n', encoding='utf-8')
        digest.close()
        self.assertEqual(printed[-1], '  ┊ still building')

    def test_logs_are_followed(self):
        logs = self.tmp / 'cr159'
        printed = []
        digest = run_claude.Digest(printed.append, logs)
        logs.mkdir()
        (logs / 'claude.jsonl').write_text('{}\n', encoding='utf-8')
        (logs / 'build-linux.log').write_text(
            '[1/2] CXX a.o\nFAILED: a.o\n', encoding='utf-8'
        )
        digest.close()
        self.assertEqual(
            printed,
            ['  ┊ ▸ build-linux.log', '  ┊ [1/2] CXX a.o', '  ┊ FAILED: a.o'],
        )

    def test_colours_are_reset_at_the_end_of_each_line(self):
        printed = []
        digest = run_claude.Digest(printed.append)
        digest.handle_line(
            json.dumps(
                _tool_result('\x1b[31mFAILED: a.o\nerror: boom\x1b[0m\nplain')
            )
        )
        self.assertEqual(
            printed,
            [
                '  ┆ \x1b[31mFAILED: a.o\x1b[0m',
                '  ┆ error: boom\x1b[0m\x1b[0m',
                '  ┆ plain',
            ],
        )

    def test_missing_output_file_prints_nothing(self):
        printed = []
        digest = run_claude.Digest(printed.append)
        digest.handle_line(json.dumps(self.started))
        digest.handle_line(json.dumps(_notification('b1')))
        self.assertEqual(printed[self.started_lines :], ['  ⏹ b1 completed'])


# A stand-in for claude: starts `sleep` in a session of its own, as Claude does
# with background commands, writes its PID, and waits.
# A stand-in for claude that leaves an orphan behind: `sh` exits at once, and
# its `sleep` exits soon after, as Xvfb does once `runtest.py` kills it.
_FAKE_CLAUDE_ORPHAN = """\
import subprocess, sys, time
subprocess.run(['sh', '-c', f'sleep 0.3 & echo $! > {sys.argv[1]}'])
time.sleep(float(sys.argv[2]))
"""

# A stand-in for claude whose background command, in a session of its own,
# takes a while to shut down on SIGTERM and records when it has, as a build
# flushing its outputs would.
_FAKE_CLAUDE_SLOW_SHUTDOWN = """\
import subprocess, sys, time
done = sys.argv[1] + '.done'
slow = subprocess.Popen(
    ['sh', '-c', f"trap 'sleep 1; echo ok > {done}; exit 0' TERM; "
                 'while :; do sleep 0.1; done'],
    start_new_session=True,
)
with open(sys.argv[1], 'w') as file:
    file.write(str(slow.pid))
time.sleep(float(sys.argv[2]))
"""

_FAKE_CLAUDE = """\
import subprocess, sys, time
sleeper = subprocess.Popen(['sleep', '300'], start_new_session=True)
with open(sys.argv[1], 'w') as file:
    file.write(str(sleeper.pid))
time.sleep(float(sys.argv[2]))
"""

_SCRIPT = Path(__file__).resolve().parent / 'run_claude.py'


def _alive(pid: int) -> bool:
    """Whether *pid* runs, zombies aside."""
    try:
        stat = Path(f'/proc/{pid}/stat').read_text(encoding='utf-8')
    except FileNotFoundError:
        return False
    return stat.rsplit(')', 1)[1].split()[0] != 'Z'


@_linux_only
class ProcessCleanupTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)
        self.pid_file = self.tmp / 'sleeper.pid'

    def _start(self, cmd_prefix, claude_seconds, fake=_FAKE_CLAUDE):
        return subprocess.Popen(
            [
                *cmd_prefix,
                sys.executable,
                str(_SCRIPT),
                '--log',
                str(self.tmp / 'claude.jsonl'),
                '--',
                sys.executable,
                '-c',
                fake,
                str(self.pid_file),
                str(claude_seconds),
            ],
            stdout=subprocess.DEVNULL,
        )

    def _sleeper(self) -> int:
        deadline = time.monotonic() + 10
        while not self.pid_file.exists() or not self.pid_file.read_text():
            self.assertLess(time.monotonic(), deadline, 'claude never started')
            time.sleep(0.05)
        pid = int(self.pid_file.read_text())
        self.addCleanup(_kill_quietly, pid)
        return pid

    def _assert_stops(self, pid):
        deadline = time.monotonic() + 15
        while _alive(pid):
            self.assertLess(time.monotonic(), deadline, f'{pid} still runs')
            time.sleep(0.05)

    def test_left_behind_processes_stop_when_claude_exits(self):
        process = self._start([], 0)
        sleeper = self._sleeper()
        self.assertEqual(process.wait(timeout=30), 0)
        self._assert_stops(sleeper)

    def test_sigterm_stops_everything(self):
        process = self._start([], 300)
        sleeper = self._sleeper()
        self.assertTrue(_alive(sleeper))
        process.send_signal(signal.SIGTERM)
        self.assertEqual(process.wait(timeout=30), 128 + signal.SIGTERM)
        self._assert_stops(sleeper)

    def test_unrelated_processes_are_left_alone(self):
        # Like another Claude session on the machine: not below the run.
        with subprocess.Popen(
            ['sleep', '300'], start_new_session=True
        ) as unrelated:
            try:
                process = self._start([], 0)
                self._sleeper()
                self.assertEqual(process.wait(timeout=30), 0)
                self.assertTrue(_alive(unrelated.pid))
            finally:
                unrelated.kill()

    def test_adopted_processes_are_collected_as_they_exit(self):
        process = self._start([], 300, _FAKE_CLAUDE_ORPHAN)
        self.addCleanup(process.wait)
        self.addCleanup(process.terminate)
        orphan = self._sleeper()
        # Gone from `/proc` altogether, not left as a zombie, while claude
        # still runs.
        deadline = time.monotonic() + 10
        while Path(f'/proc/{orphan}').exists():
            self.assertLess(time.monotonic(), deadline, f'{orphan} lingers')
            time.sleep(0.05)
        self.assertIsNone(process.poll())

    def test_processes_get_time_to_shut_down(self):
        process = self._start([], 300, _FAKE_CLAUDE_SLOW_SHUTDOWN)
        slow = self._sleeper()
        process.send_signal(signal.SIGTERM)
        self.assertEqual(process.wait(timeout=30), 128 + signal.SIGTERM)
        self._assert_stops(slow)
        done = Path(f'{self.pid_file}.done')
        self.assertTrue(
            done.exists(), 'killed before it finished shutting down'
        )

    def test_engine_dying_stops_everything(self):
        # `sh` stands for the engine, and is killed outright.
        engine = self._start(['sh', '-c', '"$@"; :', 'engine'], 300)
        sleeper = self._sleeper()
        engine.kill()
        engine.wait(timeout=30)
        self._assert_stops(sleeper)


def _kill_quietly(pid: int) -> None:
    with contextlib.suppress(ProcessLookupError):
        os.kill(pid, signal.SIGKILL)


@_linux_only
class RunTest(unittest.TestCase):
    def _run(self, lines, exit_code=0):
        script = (
            'import sys\n'
            f'sys.stdout.write({"".join(lines)!r})\n'
            f'sys.exit({exit_code})\n'
        )
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / 'cr159' / 'claude.jsonl'
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                code = run_claude.run([sys.executable, '-c', script], log)
            return code, stdout.getvalue(), log.read_text(encoding='utf-8')

    def test_logs_raw_stream_and_prints_digest(self):
        lines = [
            'a warning\n',
            json.dumps(_assistant({'type': 'text', 'text': 'Hello'})) + '\n',
            json.dumps({'type': 'result', 'subtype': 'success'}) + '\n',
        ]
        code, stdout, log = self._run(lines)
        self.assertEqual(code, 0)
        self.assertEqual(log, ''.join(lines))
        self.assertEqual(
            stdout.splitlines(),
            ['a warning', 'Hello', '■ success'],
        )

    def test_error_result_fails(self):
        lines = [
            json.dumps({'type': 'result', 'subtype': 'error', 'is_error': True})
            + '\n'
        ]
        code, _, _ = self._run(lines)
        self.assertEqual(code, 1)

    def test_claude_exit_code_is_kept(self):
        code, _, _ = self._run([], exit_code=3)
        self.assertEqual(code, 3)

    def test_background_work_stopped_at_the_end_fails(self):
        stopped = {
            'type': 'system',
            'subtype': 'task_notification',
            'task_id': 'a1',
            'status': 'stopped',
            'summary': 'Codehealth sweep a: overlap',
        }
        lines = [
            json.dumps(stopped) + '\n',
            json.dumps({'type': 'result', 'subtype': 'success'}) + '\n',
        ]
        code, stdout, _ = self._run(lines)
        self.assertEqual(code, 1)
        self.assertIn('Claude stopped 1 background tasks', stdout)
        self.assertIn('  ⏹ Codehealth sweep a: overlap', stdout)

    def test_background_work_stopped_mid_run_is_fine(self):
        stopped = {
            'type': 'system',
            'subtype': 'task_notification',
            'task_id': 'a1',
            'status': 'stopped',
            'summary': 'Stray git grep',
        }
        lines = [
            json.dumps(stopped) + '\n',
            json.dumps(_assistant({'type': 'text', 'text': 'Carrying on.'}))
            + '\n',
            json.dumps({'type': 'result', 'subtype': 'success'}) + '\n',
        ]
        code, stdout, _ = self._run(lines)
        self.assertEqual(code, 0)
        self.assertNotIn('Claude stopped', stdout)


if __name__ == '__main__':
    unittest.main()
