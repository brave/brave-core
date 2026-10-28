#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for sync_tooling.py, on real repositories."""

from __future__ import annotations

import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

import sync_tooling
from sync_tooling import SyncTooling, classify

_ENV = {
    'GIT_AUTHOR_NAME': 't',
    'GIT_AUTHOR_EMAIL': 't@t',
    'GIT_COMMITTER_NAME': 't',
    'GIT_COMMITTER_EMAIL': 't@t',
}


_CULPRIT = (
    '\n\nChromium changes:\n'
    'https://chromium.googlesource.com/chromium/src/+/'
    '5691202e214341a5d96a3296c1b9811cce19572b'
)


class ClassifyTest(unittest.TestCase):
    def test_kinds(self):
        self.assertEqual(('none', []), classify(['chrome/x.cc'], False))
        self.assertEqual(('pick', []), classify(['tools/cr/brockit.py'], False))
        self.assertEqual(('judge', []), classify(['tools/cr/brockit.py'], True))
        self.assertEqual(
            ('pick', ['tools/cr/codehealth/M159.md']),
            classify(['tools/cr/codehealth/M159.md', 'tools/cr/x.py'], False),
        )
        self.assertEqual(
            ('none', ['tools/cr/codehealth/M159.md']),
            classify(['tools/cr/codehealth/M159.md'], False),
        )
        self.assertEqual(
            ('pick', []), classify(['build/android/x.java'], False)
        )
        self.assertEqual(
            ('pick', []), classify(['tools/recipes/recipes/cr/x.py'], False)
        )
        self.assertEqual(
            ('judge', []), classify(['build/android/x.java'], True)
        )
        self.assertEqual(
            ('judge', []), classify(['tools/cr/x.py', 'browser/y.cc'], False)
        )


class SyncToolingTest(unittest.TestCase):
    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self.origin = self.root / 'origin.git'
        self._git(
            self.root, 'init', '-q', '--bare', '-b', 'master', 'origin.git'
        )
        self.work = self.root / 'work'
        self._git(self.root, 'clone', '-q', str(self.origin), 'work')
        self._commit('base', {'tools/cr/SKILL.md': 'rule one\nrule two\n'})
        self._commit('Add the recipe', {'tools/recipes/r.py': 'v1\n'})
        self._git(self.work, 'push', '-q', 'origin', 'HEAD:refs/heads/cr158')
        # cr159, based on cr158.
        self._git(self.work, 'checkout', '-q', '-b', 'cr159')
        self.shared = self._commit(
            'Shared fix', {'tools/cr/shared.py': 'shared\n'}
        )
        tool_fix = self._commit('Tool fix', {'tools/cr/brockit.py': 'fixed\n'})
        self._commit('fixup! Tool fix', {'tools/cr/brockit.py': 'fixed!\n'})
        self._commit(
            'Version CodeHealth tracker',
            {
                'tools/cr/codehealth/M159.md': 'items\n',
                'tools/cr/codehealth/scripts/x.py': 'better\n',
            },
        )
        self._commit(
            'Version CodeHealth tracker',
            {'tools/cr/codehealth/M159.md': 'more items\n'},
        )
        self._commit('Build tweak', {'build/commands/x.ts': 'x\n'})
        self._commit(
            'Bytecode redirect' + _CULPRIT, {'build/android/x.java': 'x\n'}
        )
        self._commit(
            'fixup! Bytecode redirect', {'build/android/y.java': 'y\n'}
        )
        self._commit(
            'Version lift updates',
            {'tools/cr/SKILL.md': 'rule one, cr159\nrule two\n'},
        )
        self._commit(
            'Version lift updates', {'tools/cr/SKILL.md': 'rule one, cr159\n'}
        )
        self._commit('Product change', {'chrome/x.cc': 'x\n'})
        self._commit(f'drop!{tool_fix[:11]}! Tool fix', {})
        self._commit('drop!0000000! Product change', {})
        # A fixup of a commit cr159 inherited from cr158.
        self._commit('fixup! Add the recipe', {'tools/recipes/r.py': 'v2\n'})
        self._git(self.work, 'push', '-q', 'origin', 'cr159')
        # cr158 moved on with its own skill change and the shared fix.
        self._git(self.work, 'checkout', '-q', '-B', 'cr158', 'origin/cr158')
        self._commit(
            'Version lift updates',
            {'tools/cr/SKILL.md': 'rule one, cr158\nrule two\n'},
        )
        self._git(self.work, 'cherry-pick', self.shared)
        self._git(self.work, 'push', '-q', 'origin', 'cr158')

    def _git(self, cwd: Path, *args: str) -> str:
        return subprocess.run(
            ['git', '-C', str(cwd), *args],
            check=True,
            capture_output=True,
            text=True,
            env={**os.environ, **_ENV, 'HOME': str(self.root)},
        ).stdout

    def _commit(self, subject: str, files: dict[str, str]) -> str:
        for name, text in files.items():
            path = self.work / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding='utf-8')
            self._git(self.work, 'add', name)
        self._git(self.work, 'commit', '-q', '--allow-empty', '-m', subject)
        return self._git(self.work, 'rev-parse', 'HEAD').strip()

    def _sync(self) -> sync_tooling.Outcome:
        with mock.patch.dict('os.environ', {**_ENV, 'HOME': str(self.root)}):
            return SyncTooling(self.work, 158).run()

    def test_picks_tooling_and_leaves_the_rest_for_judgement(self):
        outcome = self._sync()
        self.assertEqual(
            [
                ('Tool fix', ''),
                ('fixup! Tool fix', ''),
                ('Version CodeHealth tracker', 'without its tracker'),
                ('Build tweak', ''),
                ('fixup! Add the recipe', ''),
            ],
            [(subject, note) for _, _, subject, note in outcome.picked],
        )
        self.assertEqual(
            [('Version CodeHealth tracker', 'only its tracker')],
            [(subject, note) for _, _, subject, note in outcome.skipped],
        )
        tool_fix = next(c for _, c, s, _ in outcome.picked if s == 'Tool fix')
        self.assertEqual(
            [
                (
                    'Bytecode redirect',
                    'cites a culprit, or touches paths outside the tooling',
                ),
                ('fixup! Bytecode redirect', "its target wasn't picked"),
                ('Version lift updates', 'conflicts in tools/cr/SKILL.md'),
                (
                    'Version lift updates',
                    'touches files of an earlier pending commit',
                ),
                (
                    f'drop!{tool_fix[:11]}! Tool fix',
                    'a Brockit marker on a picked commit',
                ),
            ],
            [(subject, note) for _, _, subject, note in outcome.pending],
        )
        tree = self._git(self.work, 'ls-files').split()
        self.assertIn('tools/cr/codehealth/scripts/x.py', tree)
        self.assertNotIn('tools/cr/codehealth/M159.md', tree)
        self.assertNotIn('chrome/x.cc', tree)
        self.assertEqual('', self._git(self.work, 'status', '--porcelain'))
        self.assertIn(
            '(cherry picked from commit',
            self._git(self.work, 'log', '-1', '--format=%B'),
        )

    def test_a_second_run_finds_nothing_new_to_pick(self):
        self._sync()
        outcome = self._sync()
        self.assertEqual([], outcome.picked)
        self.assertEqual(5, len(outcome.pending))
        self.assertTrue(outcome.pending[-1][2].startswith('drop!'))

    def test_a_commit_whose_files_already_match_is_present(self):
        """Once its fixups are squashed away or the newer branch rebased,
        a pick no longer matches by patch id or trailer, but its files do."""
        outcome = self._sync()
        sync = SyncTooling(self.work, 158)
        tool_fix = next(c for _, c, s, _ in outcome.picked if s == 'Tool fix')
        redirect = next(
            c for _, c, s, _ in outcome.pending if s == 'Bytecode redirect'
        )
        self.assertTrue(sync._in_sync(tool_fix, 'origin/cr159'))
        self.assertFalse(sync._in_sync(redirect, 'origin/cr159'))

    def test_a_commit_picked_by_hand_with_changes_is_not_pending_again(self):
        outcome = self._sync()
        redirect = next(
            c for _, c, s, _ in outcome.pending if s == 'Bytecode redirect'
        )
        # Picked as an agent does, with `-x`, and changed while reconciling.
        self._git(self.work, 'cherry-pick', '-n', redirect)
        (self.work / 'build/android/x.java').write_text('x, here\n')
        self._git(self.work, 'add', 'build/android/x.java')
        full = self._git(self.work, 'rev-parse', redirect).strip()
        self._git(
            self.work,
            'commit',
            '-q',
            '-m',
            f'Bytecode redirect\n\n(cherry picked from commit {full})',
        )
        pending = [subject for _, _, subject, _ in self._sync().pending]
        self.assertNotIn('Bytecode redirect', pending)

    def test_the_newest_branch_has_nothing_to_sync(self):
        self._git(self.work, 'checkout', '-q', 'cr159')
        with mock.patch.dict('os.environ', {**_ENV, 'HOME': str(self.root)}):
            outcome = SyncTooling(self.work, 159).run()
        self.assertEqual((), (*outcome.picked, *outcome.pending))

    def _main(self) -> int:
        argv = [
            'sync_tooling.py',
            '--repo',
            str(self.work),
            '--milestone',
            '158',
        ]
        with (
            mock.patch.dict('os.environ', {**_ENV, 'HOME': str(self.root)}),
            mock.patch('sys.argv', argv),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            return sync_tooling.main()

    def test_main_reports_and_exits_2_with_pending_commits(self):
        self.assertEqual(sync_tooling.PENDING_EXIT, self._main())
        run_dir = self.work / '.autolift' / 'cr158'
        report = (run_dir / 'sync-tooling.md').read_text(encoding='utf-8')
        self.assertIn('## Pending', report)
        self.assertIn('## What to do', report)
        self.assertEqual(
            self._git(self.work, 'rev-parse', 'HEAD'),
            (run_dir / 'synced-head').read_text(encoding='utf-8'),
        )

    def test_commits_following_a_chromium_change_cr158_lacks_are_declined(self):
        chromium = self.root / 'src'
        chromium.mkdir()
        self._git(chromium, 'init', '-q', '-b', 'main')
        self._git(chromium, 'commit', '-q', '--allow-empty', '-m', 'old')
        self._git(chromium, 'tag', '158.0.1.0')
        old = self._git(chromium, 'rev-parse', 'HEAD').strip()
        self._git(chromium, 'commit', '-q', '--allow-empty', '-m', 'newer')
        newer = self._git(chromium, 'rev-parse', 'HEAD').strip()

        def link(sha: str) -> str:
            return (
                '\n\nChromium changes:\n'
                f'https://chromium.googlesource.com/chromium/src/+/{sha}'
            )

        self._git(self.work, 'checkout', '-q', 'cr159')
        self._commit('Follows newer' + link(newer), {'build/a.gn': 'a\n'})
        self._commit('fixup! Follows newer', {'build/a.gn': 'b\n'})
        self._commit('Follows old' + link(old), {'build/c.gn': 'c\n'})
        self._git(self.work, 'push', '-q', 'origin', 'cr159')
        self._git(self.work, 'checkout', '-q', 'cr158')
        self._commit(
            'Version bump',
            {
                'package.json': json.dumps(
                    {'config': {'projects': {'chrome': {'tag': '158.0.1.0'}}}}
                )
            },
        )
        with mock.patch.dict('os.environ', {**_ENV, 'HOME': str(self.root)}):
            outcome = SyncTooling(self.work, 158, chromium=chromium).run()
        self.assertEqual(
            [
                ('Follows newer', 'cites no culprit 158.0.1.0 has'),
                ('fixup! Follows newer', 'its target was declined'),
            ],
            [(subject, note) for _, _, subject, note in outcome.declined],
        )
        self.assertIn(
            'Follows old', [subject for _, _, subject, _ in outcome.pending]
        )


if __name__ == '__main__':
    unittest.main()
