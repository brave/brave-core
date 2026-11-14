#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for sweep.py."""

from __future__ import annotations

import contextlib
import subprocess
import tempfile
import unittest
from dataclasses import dataclass, field
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import knowledge_base
import sweep
from knowledge_base import Change
from sweep import Commit, RangeDump, Sweep

_RULES = 'tools/cr/codehealth/rules/s'

_METADATA = rf'''
agentic_review {{
  capability_groups {{
    path_expression: ".*\\.java"
    path_expression_exclusion: ".*Test\\.java"
    diff_patterns: "^-.*\\bfoo\\("
    diff_patterns: "^-package\\s"
    brave_mentions: "(\\w+)\\s*\\("
    rules: "{_RULES}/a.md"
    rules: "{_RULES}/b.md"
  }}
  capability_groups {{
    tool: "echo {{old}} {{new}}"
    rules: "{_RULES}/tool.md"
  }}
  capability_groups {{
    subject_patterns: "^Deprecate"
    rules: "{_RULES}/subjects.md"
  }}
  capability_groups {{
    path_expression: "ios/.*"
    overridden_by_brave: true
    rules: "{_RULES}/a.md"
  }}
  capability_groups {{
    rules: "{_RULES}/whole.md"
  }}
}}
'''


@dataclass(frozen=True)
class FakeBrave:
    """A `BraveTree` with the words and overridden files a test gives it."""

    words: set[str] = field(default_factory=set)
    overridden: set[str] = field(default_factory=set)
    adapted: set[str] = field(default_factory=set)


def _commit(sha: str, subject: str, path: str = '', *lines: str) -> Commit:
    diff = ''
    if path:
        diff = f'diff --git a/{path} b/{path}\n@@ -1 +1 @@\n' + ''.join(
            f'{line}\n' for line in lines
        )
    return Commit(sha, Change.parse(diff, subject=subject))


class RangeDumpTest(unittest.TestCase):
    """`git log -p` on a real repository: edits, renames and deletions."""

    def _git(self, *args: str) -> None:
        subprocess.run(
            ['git', '-C', str(self.root), *args],
            check=True,
            capture_output=True,
            env={
                'GIT_AUTHOR_NAME': 't',
                'GIT_AUTHOR_EMAIL': 't@t',
                'GIT_COMMITTER_NAME': 't',
                'GIT_COMMITTER_EMAIL': 't@t',
                'HOME': str(self.root),
                'PATH': '/usr/bin:/bin',
            },
        )

    def _write(self, path: str, text: str) -> None:
        (self.root / path).write_text(text, encoding='utf-8', newline='')

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self._git('init', '-q', '-b', 'main')
        (self.root / 'a').mkdir()
        self._write(
            'a/Old.java',
            'package a;\n\nclass Old {\n'
            + ''.join(f'  int f{i};\n' for i in range(20))
            + '}\n',
        )
        self._write('gone.txt', '--- not a header\n')
        self._git('add', '.')
        self._git('commit', '-q', '-m', 'base')
        self._git('tag', 'old')
        (self.root / 'b').mkdir()
        self._git('mv', 'a/Old.java', 'b/Old.java')
        text = (self.root / 'b/Old.java').read_bytes().decode('utf-8')
        self._write('b/Old.java', text.replace('package a;', 'package b;'))
        self._git('rm', '-q', 'gone.txt')
        self._git(
            'commit',
            '-q',
            '-am',
            'move\n\nQuotes a diff:\ndiff --git a/x b/x\n-gone\n\nBug: 1',
        )
        self._git('tag', 'new')
        self.enterContext(
            mock.patch.object(
                sweep.repository, 'chromium', SimpleNamespace(root=self.root)
            )
        )

    def test_commits(self):
        (commit,) = RangeDump('old', 'new', self.root).commits()
        self.assertEqual('move', commit.subject)
        change = commit.change
        self.assertEqual(
            ['Quotes a diff:', 'diff --git a/x b/x', '-gone', '', 'Bug: 1'],
            change.body[:5],
        )
        self.assertEqual(['b/Old.java', 'gone.txt'], sorted(change.files))
        moved = change.files['b/Old.java']
        self.assertEqual(['-package a;', '+package b;'], moved.lines)
        self.assertEqual('a/Old.java', moved.old_path)
        self.assertEqual(['---- not a header'], change.files['gone.txt'].lines)
        # Its files are read as the commit leaves them.
        self.assertIn('package b;', change.read('b/Old.java'))
        self.assertIsNone(change.read('gone.txt'))

    def test_changes_name_each_line_commit_and_path(self):
        def run_git(*args, **_):
            return subprocess.run(
                ['git', '-C', str(self.root), *args],
                check=True,
                capture_output=True,
                text=True,
            ).stdout

        self.enterContext(
            mock.patch.object(
                sweep.repository,
                'chromium',
                SimpleNamespace(root=self.root, run_git=run_git),
            )
        )
        out = self.root / 'dump'
        out.mkdir()
        RangeDump('old', 'new', out).write_files()
        sha = run_git('rev-parse', '--short=12', 'new').strip()
        self.assertEqual(
            [
                f'{sha} b/Old.java:-package a;',
                f'{sha} b/Old.java:+package b;',
                f'{sha} gone.txt:---- not a header',
            ],
            (out / 'changes').read_text(encoding='utf-8').splitlines(),
        )


class SweepTest(unittest.TestCase):
    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        tmp = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        (tmp / '.git').mkdir()
        self.rules = tmp / _RULES
        self.rules.mkdir(parents=True)
        (self.rules / 'COMMON_METADATA').write_text(
            _METADATA, encoding='utf-8', newline=''
        )
        for name in ('a', 'b', 'subjects', 'tool', 'whole', 'unmatched'):
            (self.rules / f'{name}.md').write_text(
                f'---\nname: s/{name}\nrubric: s\nseverity: optional\n---\n',
                encoding='utf-8',
                newline='',
            )
        self.out = tmp / 'out'
        commits = [
            _commit('111', 'renames foo', 'x/A.java', '-  void foo(int a);'),
            _commit('222', 'unrelated', 'x/A.java', '-  void bar();'),
            _commit('333', 'overridden', 'ios/o.mm', '+x'),
            _commit('444', 'Deprecate Foo', 'x/B.java', '+y'),
        ]
        self.dump = SimpleNamespace(
            old='o', new='n', out=self.out, commits=lambda: iter(commits)
        )
        brave = FakeBrave({'foo'}, {'ios/o.mm'})
        for target, attribute, value in (
            (sweep, 'RULES_DIR', self.rules.parent),
            (knowledge_base, 'brave_tree', lambda: brave),
            (sweep.repository, 'brave', SimpleNamespace(root=None)),
        ):
            stack.enter_context(mock.patch.object(target, attribute, value))
        knowledge_base._RULE_CACHE.clear()
        self.addCleanup(knowledge_base._RULE_CACHE.clear)

    def _read(self, name: str) -> str:
        return (self.out / 'rules/s' / name).read_bytes().decode('utf-8')

    def _run(self) -> list[tuple[str, str, str]]:
        runner = Sweep(self.dump, None)
        return runner.write(runner.run())

    def test_run_and_write(self):
        self.assertEqual(
            [
                ('s', 'a', '2 commits'),
                ('s', 'b', '1 commits'),
                ('s', 'subjects', '1 commits'),
                ('s', 'tool', 'tool'),
                ('s', 'unmatched', '0 commits'),
                ('s', 'whole', 'whole range'),
            ],
            self._run(),
        )
        a = self._read('a.candidates')
        self.assertIn('111 renames foo', a)
        self.assertIn('333 overridden', a)
        self.assertNotIn('222', a)
        self.assertIn('o n', self._read('tool.candidates'))
        self.assertEqual(
            '444 Deprecate Foo\n', self._read('subjects.candidates')
        )
        self.assertEqual(
            'Read the whole range.\n', self._read('whole.candidates')
        )

    def test_rules_sharing_a_list(self):
        self.dump.commits = lambda: iter(
            [_commit('111', 'renames foo', 'x/A.java', '-  void foo(int a);')]
        )
        self.assertIn(('s', 'b', '1 commits, as a'), self._run())
        self.assertEqual('Same as a.candidates.\n', self._read('b.candidates'))

    def test_a_change_and_its_revert_are_marked(self):
        self.dump.commits = lambda: iter(
            [
                _commit(
                    '111', 'renames foo', 'x/A.java', '-  void foo(int a);'
                ),
                _commit(
                    '555',
                    'Revert "renames foo"',
                    'x/A.java',
                    '-  void foo();',
                    '+  void foo(int a);',
                ),
            ]
        )
        self._run()
        a = self._read('a.candidates')
        self.assertIn('111 renames foo (reverted in range)', a)
        self.assertIn('555 Revert "renames foo" (reverts a commit in range)', a)

    def test_a_shared_list_is_under_its_groups_first_rule(self):
        (self.rules / 'COMMON_METADATA').write_text(
            'agentic_review {\n'
            '  capability_groups {\n'
            '    tool: "echo {old} {new}"\n'
            f'    rules: "{_RULES}/tool.md"\n'
            f'    rules: "{_RULES}/a.md"\n'
            '  }\n'
            '}\n',
            encoding='utf-8',
            newline='',
        )
        rows = self._run()
        self.assertIn(('s', 'tool', 'tool'), rows)
        self.assertIn(('s', 'a', 'tool, as tool'), rows)
        self.assertIn('o n', self._read('tool.candidates'))
        self.assertEqual(
            'Same as tool.candidates.\n', self._read('a.candidates')
        )

    def test_sweep_filter(self):
        runner = Sweep(self.dump, ['other'])
        self.assertEqual([], runner.write(runner.run()))


if __name__ == '__main__':
    unittest.main()
