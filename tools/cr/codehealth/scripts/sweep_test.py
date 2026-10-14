#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for sweep.py."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import shutil
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import sweep
from sweep import CapabilityGroup, Commit, Metadata, RangeDump, Sweep

_METADATA = r'''
# A comment.
agentic_review {
  capability_groups {
    path_expression: ".*\\.java"
    exclude_path_expression: ".*Test\\.java"
    content_patterns: "^-.*\\bfoo\\("
    content_patterns: "^-package\\s"
    brave_mentions: "(\\w+)\\s*\\("
    rules: "tools/cr/codehealth/rules/s/a.md"
    rules: "tools/cr/codehealth/rules/s/b.md"
  }
  capability_groups {
    tool: "echo {old} {new}"
    rules: "tools/cr/codehealth/rules/s/tool.md"
  }
  capability_groups {
    subject_patterns: "^Deprecate"
    rules: "tools/cr/codehealth/rules/s/subjects.md"
  }
  capability_groups {
    path_expression: "ios/.*"
    overridden_by_brave: true
    rules: "tools/cr/codehealth/rules/s/a.md"
  }
}
'''


@dataclass(frozen=True)
class FakeBrave:
    """A `BraveTree` with the words and overridden files a test gives it."""

    words: set[str] = field(default_factory=set)
    overridden: set[str] = field(default_factory=set)
    adapted: set[str] = field(default_factory=set)


class MetadataTest(unittest.TestCase):
    def test_parse(self):
        groups = Metadata.parse(_METADATA)
        self.assertEqual(4, len(groups))
        first, tool, subjects, overridden = (groups[i] for i in range(4))
        self.assertEqual(['^Deprecate'], subjects.subject_patterns)
        self.assertEqual(r'.*\.java', first.path_expression)
        self.assertEqual(
            [r'^-.*\bfoo\(', r'^-package\s'], first.content_patterns
        )
        self.assertEqual(2, len(first.rules))
        self.assertEqual('echo {old} {new}', tool.tool)
        self.assertTrue(overridden.overridden_by_brave)
        self.assertFalse(first.overridden_by_brave)

    def test_unknown_field(self):
        with self.assertRaisesRegex(ValueError, 'unknown field bogus'):
            Metadata.parse('a { capability_groups { bogus: "x" } }')

    def test_unparseable(self):
        with self.assertRaisesRegex(ValueError, 'cannot parse'):
            Metadata.parse('a { capability_groups { = } }')


class CapabilityGroupTest(unittest.TestCase):
    def test_path_exclude_and_content(self):
        group = CapabilityGroup(
            path_expression=r'.*\.java',
            exclude_path_expression=r'.*Test\.java',
            content_patterns=[r'^-.*\bfoo\('],
        )
        commit = Commit(
            'abc',
            's',
            {
                'a/A.java': ['-  void foo(int a);', '+  void foo();'],
                'a/ATest.java': ['-  void foo(int a);'],
                'a/B.java': ['-  void bar();'],
                'a/A.cc': ['-  void foo(int a);'],
            },
        )
        self.assertEqual(
            {'a/A.java': ['-  void foo(int a);']},
            group.files(commit, FakeBrave()),
        )

    def test_no_patterns_keep_every_line(self):
        group = CapabilityGroup(path_expression='ios/.*')
        commit = Commit('abc', 's', {'ios/a.mm': ['+x', '-y']})
        self.assertEqual(
            {'ios/a.mm': ['+x', '-y']}, group.files(commit, FakeBrave())
        )

    def test_overridden_by_brave(self):
        group = CapabilityGroup(
            path_expression='ios/.*', overridden_by_brave=True
        )
        commit = Commit('abc', 's', {'ios/a.mm': ['+x'], 'ios/b.mm': ['+x']})
        self.assertEqual(
            ['ios/b.mm'],
            list(group.files(commit, FakeBrave(overridden={'ios/b.mm'}))),
        )

    def test_bytecode_adapted(self):
        group = CapabilityGroup(
            path_expression=r'.*\.java', bytecode_adapted=True
        )
        settings = (
            'chrome/android/java/src/org/chromium/chrome/browser/settings'
        )
        commit = Commit(
            'abc',
            's',
            {
                f'{settings}/MainSettings.java': ['+x'],
                f'{settings}/A.java': ['+x'],
            },
        )
        brave = FakeBrave(
            adapted={'org/chromium/chrome/browser/settings/MainSettings'}
        )
        self.assertEqual(
            [f'{settings}/MainSettings.java'],
            list(group.files(commit, brave)),
        )
        self.assertTrue(
            CapabilityGroup.from_fields(
                {'bytecode_adapted': 'true'}
            ).bytecode_adapted
        )

    def test_required_patterns(self):
        group = CapabilityGroup(
            content_patterns=[r'^-.*ENABLED_BY_DEFAULT'],
            required_patterns=[r'^\+.*DISABLED_BY_DEFAULT'],
        )
        commit = Commit(
            'abc',
            's',
            {
                'flip.cc': [
                    '-F(kA, ENABLED_BY_DEFAULT);',
                    '+F(kA, DISABLED_BY_DEFAULT);',
                ],
                'removal.cc': ['-F(kB, ENABLED_BY_DEFAULT);'],
            },
        )
        self.assertEqual(
            {'flip.cc': ['-F(kA, ENABLED_BY_DEFAULT);']},
            group.files(commit, FakeBrave()),
        )

    def test_brave_mentions_in_lines(self):
        group = CapabilityGroup(brave_mentions=r'(\w+)\s*\(')
        commit = Commit(
            'abc',
            's',
            {
                'A.java': ['-void used(int a);', '-void unused();'],
                'B.java': ['-void unused();'],
            },
        )
        self.assertEqual(
            {'A.java': ['-void used(int a);']},
            group.files(commit, FakeBrave(words={'used', 'int'})),
        )

    def test_brave_mentions_in_the_path_keep_every_line(self):
        group = CapabilityGroup(brave_mentions=r'/(\w+)\.java$')
        commit = Commit(
            'abc',
            's',
            {
                'a/Used.java': ['-package a;'],
                'a/Unused.java': ['-package a;'],
            },
        )
        self.assertEqual(
            {'a/Used.java': ['-package a;']},
            group.files(commit, FakeBrave(words={'Used'})),
        )

    def test_renames_match_by_old_path(self):
        group = CapabilityGroup(
            path_expression='old/.*', overridden_by_brave=True
        )
        commit = Commit(
            'abc',
            's',
            {'new/A.java': ['-x']},
            renamed={'new/A.java': ['old/A.java']},
        )
        self.assertEqual(
            {'new/A.java': ['-x']},
            group.files(commit, FakeBrave(overridden={'old/A.java'})),
        )

    def test_message_patterns(self):
        group = CapabilityGroup(
            subject_patterns=[r'(?i)deprecat'],
            body_patterns=[r'^Fixed: \d+'],
            exclude_subject_expression=r'^\[ash\]',
        )
        self.assertFalse(group.matches_files)
        hit = Commit('a', 'Deprecate Foo')
        body = Commit('b', 'Other', body=['x', 'Fixed: 12'])
        excluded = Commit('c', '[ash] Deprecate Bar')
        miss = Commit('d', 'Other', body=['Bug: 1'])
        self.assertEqual([], group.message(hit))
        self.assertEqual(['Fixed: 12'], group.message(body))
        self.assertIsNone(group.message(excluded))
        self.assertIsNone(group.message(miss))

    def test_no_message_patterns(self):
        group = CapabilityGroup(path_expression='ios/.*')
        self.assertTrue(group.matches_files)
        self.assertEqual([], group.message(Commit('a', 's')))


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
        self.root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
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
        self.assertEqual(
            ['Quotes a diff:', 'diff --git a/x b/x', '-gone', '', 'Bug: 1'],
            commit.body[:5],
        )
        self.assertEqual(['b/Old.java', 'gone.txt'], sorted(commit.changes))
        self.assertEqual(
            ['-package a;', '+package b;'], commit.changes['b/Old.java']
        )
        self.assertEqual(['---- not a header'], commit.changes['gone.txt'])
        self.assertEqual({'b/Old.java': ['a/Old.java']}, commit.renamed)

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
        tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, tmp)
        rules = tmp / 'rules/s'
        rules.mkdir(parents=True)
        (rules / 'COMMON_METADATA').write_text(
            _METADATA, encoding='utf-8', newline=''
        )
        for name in ('a', 'b', 'subjects', 'tool', 'unmatched'):
            (rules / f'{name}.md').write_text('', encoding='utf-8', newline='')
        self.out = tmp / 'out'
        commits = [
            Commit('111', 'renames foo', {'x/A.java': ['-  void foo(int a);']}),
            Commit('222', 'unrelated', {'x/A.java': ['-  void bar();']}),
            Commit('333', 'overridden', {'ios/o.mm': ['+x']}),
            Commit('444', 'Deprecate Foo', {'x/B.java': ['+y']}),
        ]
        self.dump = SimpleNamespace(
            old='o', new='n', out=self.out, commits=lambda: iter(commits)
        )
        for target, value in (
            ('RULES_DIR', tmp / 'rules'),
            ('BraveTree', lambda: FakeBrave({'foo'}, {'ios/o.mm'})),
        ):
            self.enterContext(mock.patch.object(sweep, target, value))

    def _read(self, name: str) -> str:
        return (self.out / 'rules/s' / name).read_bytes().decode('utf-8')

    def test_run_and_write(self):
        with mock.patch.object(
            sweep.repository, 'brave', SimpleNamespace(root=None)
        ):
            runner = Sweep(self.dump, None)
            rows = runner.write(runner.run())
        self.assertEqual(
            [
                ('s', 'a', '2 commits'),
                ('s', 'b', '1 commits'),
                ('s', 'subjects', '1 commits'),
                ('s', 'tool', 'tool'),
                ('s', 'unmatched', 'whole range'),
            ],
            rows,
        )
        a = self._read('a.candidates')
        self.assertIn('111 renames foo', a)
        self.assertIn('333 overridden', a)
        self.assertNotIn('222', a)
        self.assertIn('o n', self._read('tool.candidates'))
        self.assertEqual(
            '444 Deprecate Foo\n', self._read('subjects.candidates')
        )

    def test_rules_sharing_a_list(self):
        self.dump.commits = lambda: iter(
            [
                Commit(
                    '111', 'renames foo', {'x/A.java': ['-  void foo(int a);']}
                )
            ]
        )
        with mock.patch.object(
            sweep.repository, 'brave', SimpleNamespace(root=None)
        ):
            runner = Sweep(self.dump, None)
            rows = runner.write(runner.run())
        self.assertIn(('s', 'b', '1 commits, as a'), rows)
        self.assertEqual('Same as a.candidates.\n', self._read('b.candidates'))

    def test_a_change_and_its_revert_are_marked(self):
        self.dump.commits = lambda: iter(
            [
                Commit(
                    '111', 'renames foo', {'x/A.java': ['-  void foo(int a);']}
                ),
                Commit(
                    '555',
                    'Revert "renames foo"',
                    {'x/A.java': ['-  void foo();', '+  void foo(int a);']},
                ),
            ]
        )
        with mock.patch.object(
            sweep.repository, 'brave', SimpleNamespace(root=None)
        ):
            runner = Sweep(self.dump, None)
            runner.write(runner.run())
        a = self._read('a.candidates')
        self.assertIn('111 renames foo (reverted in range)', a)
        self.assertIn('555 Revert "renames foo" (reverts a commit in range)', a)

    def test_a_shared_list_is_under_its_groups_first_rule(self):
        metadata = self.out.parent / 'rules/s/COMMON_METADATA'
        metadata.write_text(
            'agentic_review {\n'
            '  capability_groups {\n'
            '    tool: "echo {old} {new}"\n'
            '    rules: "tools/cr/codehealth/rules/s/tool.md"\n'
            '    rules: "tools/cr/codehealth/rules/s/a.md"\n'
            '  }\n'
            '}\n',
            encoding='utf-8',
            newline='',
        )
        with mock.patch.object(
            sweep.repository, 'brave', SimpleNamespace(root=None)
        ):
            runner = Sweep(self.dump, None)
            rows = runner.write(runner.run())
        self.assertIn(('s', 'tool', 'tool'), rows)
        self.assertIn(('s', 'a', 'tool, as tool'), rows)
        self.assertIn('o n', self._read('tool.candidates'))
        self.assertEqual(
            'Same as tool.candidates.\n', self._read('a.candidates')
        )

    def _holds(self, path: str, text: str, pattern: str) -> bool:
        chromium = SimpleNamespace(run_git=lambda *args, **kwargs: text)
        with mock.patch.object(sweep.repository, 'chromium', chromium):
            return Sweep(self.dump, None)._holds(
                CapabilityGroup(ast_pattern=pattern), path
            )

    def test_ast_pattern_reads_headers_as_cxx(self):
        header = 'namespace foo {\nclass BASE_EXPORT Bar final {};\n}\n'
        self.assertTrue(self._holds('a/b.h', header, 'class $C final {}'))
        self.assertFalse(self._holds('a/b.h', header, 'class Baz final {}'))

    def test_ast_pattern_on_gn(self):
        gn = 'source_set("x") {\n  deps = [ "//base" ]\n}\n'
        self.assertTrue(self._holds('a/BUILD.gn', gn, '"//base"'))

    def test_ast_pattern_without_a_language(self):
        self.assertFalse(self._holds('a/b.txt', 'x', 'x'))

    def test_sweep_filter(self):
        runner = Sweep(self.dump, ['other'])
        self.assertEqual([], runner.write(runner.run()))


if __name__ == '__main__':
    unittest.main()
