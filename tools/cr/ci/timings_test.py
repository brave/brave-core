#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import datetime
import unittest

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import timings  # noqa: E402  -- after _boot

_LOG = '\n'.join(
    [
        '[2026-11-23T15:13:09.922Z] >>> git push gerrit +abc:refs/heads/main',
        '[2026-11-23T15:18:01.380Z] remote: ',
        'a line without a timestamp',
        '[2026-11-23T15:20:12.346Z] Pushing 1 tag(s) in batches of 200',
        '[2026-11-23T15:20:12.346Z] >>> git push gerrit +refs/tags/1',
        '[2026-11-23T15:21:42.387Z] Cancelling nested steps due to timeout',
    ]
)


class CommandsTest(unittest.TestCase):
    def test_times_each_command_until_the_next_or_the_last_line(self):
        found = timings.commands(_LOG)
        self.assertEqual(
            [(command.line, round(command.seconds)) for command in found],
            [
                ('>>> git push gerrit +abc:refs/heads/main', 422),
                ('>>> git push gerrit +refs/tags/1', 90),
            ],
        )
        self.assertEqual(
            found[0].started,
            datetime.datetime(
                2026, 11, 23, 15, 13, 9, 922000, datetime.timezone.utc
            ),
        )

    def test_reads_recipe_shell_and_github_actions_lines(self):
        log = '\n'.join(
            [
                '2026-11-23T10:00:00.1234567Z INFO:root: >>>> git fetch origin',
                '2026-11-23T10:00:30.0000000Z + engine.py cr/lift',
                '2026-11-23T10:01:00.0000000Z done',
            ]
        )
        self.assertEqual(
            [round(command.seconds) for command in timings.commands(log)],
            [30, 30],
        )

    def test_has_no_commands_without_timestamps(self):
        self.assertEqual(timings.commands('>>> git push\n+ make\n'), [])

    def test_matching_keeps_the_commands_whose_line_matches(self):
        found = timings.matching(_LOG, r'refs/tags/')
        self.assertEqual([round(command.seconds) for command in found], [90])


class FormatSecondsTest(unittest.TestCase):
    def test_formats_minutes_and_seconds(self):
        self.assertEqual(timings.format_seconds(422.4), '7m02s')
        self.assertEqual(timings.format_seconds(42), '42s')


if __name__ == '__main__':
    unittest.main()
