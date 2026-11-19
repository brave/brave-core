#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for stage.py."""

import contextlib
import json
import tempfile
import unittest
from pathlib import Path

from stage import main, stage


class StageTest(unittest.TestCase):
    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        temp = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self.root = temp.resolve() / 'src'
        self.out = self.root / 'out' / 'Release'
        self.destination = temp / 'stage'
        for file in (
            'out/Release/foo_unittests',
            'brave/test/data/a.txt',
            'brave/test/data/nested/b.txt',
        ):
            (self.root / file).parent.mkdir(parents=True, exist_ok=True)
            (self.root / file).write_text(file, encoding='utf-8')

    def staged(self) -> list[str]:
        return sorted(
            p.relative_to(self.destination).as_posix()
            for p in self.destination.rglob('*')
            if p.is_file()
        )

    def test_keeps_places_relative_to_root(self):
        count = stage(
            self.root,
            self.out,
            ['./foo_unittests', '../../brave/test/data/'],
            self.destination,
        )
        self.assertEqual(count, 3)
        self.assertEqual(
            self.staged(),
            [
                'brave/test/data/a.txt',
                'brave/test/data/nested/b.txt',
                'out/Release/foo_unittests',
            ],
        )
        self.assertEqual(
            (self.destination / 'brave/test/data/a.txt').read_text(
                encoding='utf-8'
            ),
            'brave/test/data/a.txt',
        )

    def test_overlapping_paths_stage_once(self):
        count = stage(
            self.root,
            self.root,
            ['brave/test/data/', 'brave/test/data/a.txt'],
            self.destination,
        )
        self.assertEqual(count, 2)

    def test_outside_root(self):
        with self.assertRaisesRegex(ValueError, 'outside'):
            stage(self.root, self.out, ['../../../etc'], self.destination)

    def test_missing(self):
        with self.assertRaisesRegex(ValueError, 'does not exist'):
            stage(self.root, self.out, ['missing'], self.destination)

    def test_main(self):
        paths_json = self.root.parent / 'paths.json'
        paths_json.write_text(json.dumps(['./foo_unittests']), encoding='utf-8')
        argv = [
            '--root',
            str(self.root),
            '--cwd',
            str(self.out),
            '--stage',
            str(self.destination),
            '--paths-json',
            str(paths_json),
        ]
        self.assertEqual(main(argv), 0)
        self.assertEqual(self.staged(), ['out/Release/foo_unittests'])

        paths_json.write_text(json.dumps(['missing']), encoding='utf-8')
        self.assertEqual(main(argv), 1)


if __name__ == '__main__':
    unittest.main()
