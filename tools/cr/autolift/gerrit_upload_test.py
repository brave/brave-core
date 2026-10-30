#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import contextlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

import gerrit_upload
from gerrit_upload import Change, ChainCommit


def _change_id(n: int) -> str:
    return f'I{n:040x}'


class ChainProblemsTest(unittest.TestCase):
    def test_a_chain_of_one_change_id_each_is_fine(self):
        self.assertEqual(
            gerrit_upload.chain_problems(
                [
                    ChainCommit('a' * 40, 'A', (_change_id(1),)),
                    ChainCommit('b' * 40, 'B', (_change_id(2),)),
                ]
            ),
            [],
        )

    def test_missing_several_and_shared_change_ids(self):
        self.assertEqual(
            gerrit_upload.chain_problems(
                [
                    ChainCommit('a' * 40, 'A', ()),
                    ChainCommit('b' * 40, 'B', (_change_id(1), _change_id(2))),
                    ChainCommit('c' * 40, 'C', (_change_id(1),)),
                ]
            ),
            [
                f'{"a" * 12} (A) has no Change-Id.',
                f'{"b" * 12} (B) has 2 Change-Ids.',
                f'{"c" * 12} (C) has the Change-Id of {"b" * 12} (B): '
                f'{_change_id(1)}.',
            ],
        )


class GerritTest(unittest.TestCase):
    def test_ssh_command_quotes_its_arguments(self):
        gerrit = gerrit_upload.Gerrit(
            'ssh://bot@gerrit.example.com:29418/brave-browser/brave-core.git'
        )
        self.assertEqual(gerrit.project, 'brave-browser/brave-core')
        self.assertEqual(
            gerrit.ssh_argv('review', '--message', "It's gone", '12,3'),
            [
                'ssh',
                '-o',
                'BatchMode=yes',
                '-p',
                '29418',
                'bot@gerrit.example.com',
                'gerrit',
                'review',
                '--message',
                "'It'\"'\"'s gone'",
                '12,3',
            ],
        )

    def test_changes_are_read_page_by_page(self):
        gerrit = gerrit_upload.Gerrit('ssh://gerrit.example.com/project')
        row = (
            '{{"id": "{id}", "number": {n}, "status": "NEW", '
            '"currentPatchSet": {{"number": 2}}}}'
        )
        pages = [
            row.format(id=_change_id(1), n=1)
            + '\n{"type": "stats", "moreChanges": true}\n',
            row.format(id=_change_id(2), n=2)
            + '\n{"type": "stats", "moreChanges": false}\n',
        ]
        with mock.patch.object(gerrit, '_ssh', side_effect=pages) as ssh:
            changes = gerrit.changes('cr160')
        self.assertEqual(
            changes,
            [
                Change(_change_id(1), 1, 2, 'NEW'),
                Change(_change_id(2), 2, 2, 'NEW'),
            ],
        )
        self.assertIn('--start=1', ssh.call_args_list[1].args)
        self.assertIn(
            'project:project branch:cr160 (status:open OR status:abandoned)',
            ssh.call_args_list[0].args,
        )


class _FakeGerrit(gerrit_upload.Gerrit):
    """A bare repository for the pushes, and a list for the CLs."""

    def __init__(self, url: str, changes: list[Change]):
        super().__init__(url)
        self._changes = changes
        self.abandoned: list[int] = []
        self.restored: list[int] = []

    def changes(self, branch: str) -> list[Change]:
        return self._changes

    def abandon(self, change: Change, message: str) -> None:
        self.abandoned.append(change.number)

    def restore(self, change: Change, message: str) -> None:
        self.restored.append(change.number)


class UploadTest(unittest.TestCase):
    """`cr160` on `master`, as `publish` leaves the checkout, uploaded to a
    bare repository standing in for Gerrit."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        stack.enter_context(
            mock.patch.dict(
                os.environ,
                {'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1'},
            )
        )
        self.dir = root / 'brave'
        self.origin = root / 'origin.git'
        self.gerrit_dir = root / 'gerrit.git'
        for bare in (self.origin, self.gerrit_dir):
            self._git(root, 'init', '-q', '--bare', str(bare))
        # Gerrit takes the push options of `refs/for/` pushes.
        self._git(
            self.gerrit_dir, 'config', 'receive.advertisePushOptions', 'true'
        )
        self.dir.mkdir()
        self._git(self.dir, 'init', '-q', '-b', 'master')
        self._git(self.dir, 'config', 'user.email', 'a@b.c')
        self._git(self.dir, 'config', 'user.name', 'A')
        self._git(self.dir, 'remote', 'add', 'origin', str(self.origin))
        self.fork = self._commit('Base')
        self._git(self.dir, 'checkout', '-q', '-b', 'cr160')
        self.chain = [
            self._commit(f'Fix {n}', change_id=_change_id(n)) for n in (1, 2)
        ]
        self._git(self.dir, 'checkout', '-q', 'master')
        self.base = self._commit('Later on master')
        self._git(self.dir, 'push', '-q', 'origin', 'master', 'cr160')
        self._git(self.dir, 'fetch', '-q', 'origin')

    def _git(self, cwd: Path, *args: str) -> str:
        return subprocess.run(
            ['git', '-C', str(cwd), *args],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()

    def _commit(self, subject: str, change_id: str | None = None) -> str:
        message = subject + (f'\n\nChange-Id: {change_id}' if change_id else '')
        self._git(self.dir, 'commit', '-q', '--allow-empty', '-m', message)
        return self._git(self.dir, 'rev-parse', 'HEAD')

    def _gerrit_ref(self, ref: str) -> str | None:
        output = self._git(
            self.gerrit_dir, 'for-each-ref', '--format=%(objectname)', ref
        )
        return output or None

    def _upload(
        self, gerrit: _FakeGerrit, dry_run: bool = False
    ) -> tuple[int, list[str]]:
        lines: list[str] = []
        status = gerrit_upload.upload(
            cwd=self.dir,
            gerrit=gerrit,
            branch='cr160',
            base='origin/master',
            head='origin/cr160',
            dry_run=dry_run,
            log=lines.append,
        )
        return status, lines

    def test_uploads_the_chain_on_the_fork_point(self):
        gerrit = _FakeGerrit(
            str(self.gerrit_dir),
            [
                Change(_change_id(1), 11, 1, 'NEW'),
                Change(_change_id(2), 12, 3, 'ABANDONED'),
                Change(_change_id(3), 13, 2, 'NEW'),
                Change(_change_id(4), 14, 1, 'ABANDONED'),
            ],
        )

        status, lines = self._upload(gerrit)

        self.assertEqual(status, 0, msg=lines)
        self.assertEqual(self._gerrit_ref('refs/heads/master'), self.base)
        self.assertEqual(self._gerrit_ref('refs/heads/cr160'), self.fork)
        self.assertEqual(self._gerrit_ref('refs/for/cr160'), self.chain[-1])
        # Back on the branch, and left it.
        self.assertEqual(gerrit.restored, [12])
        self.assertEqual(gerrit.abandoned, [13])

    def test_a_second_upload_changes_nothing_on_the_branches(self):
        gerrit = _FakeGerrit(str(self.gerrit_dir), [])
        self._upload(gerrit)

        status, lines = self._upload(gerrit)

        self.assertEqual(status, 0, msg=lines)
        self.assertFalse(
            any('Moving' in line or 'Bringing' in line for line in lines)
        )

    def test_a_cr_base_is_not_pushed(self):
        """It is a chain of its own on Gerrit, whose branch stays at its own
        fork point."""
        self._git(self.dir, 'push', '-q', 'origin', 'master:cr159')
        self._git(self.dir, 'fetch', '-q', 'origin')
        gerrit = _FakeGerrit(str(self.gerrit_dir), [])

        status = gerrit_upload.upload(
            cwd=self.dir,
            gerrit=gerrit,
            branch='cr160',
            base='origin/cr159',
            head='origin/cr160',
            log=lambda _: None,
        )

        self.assertEqual(status, 0)
        self.assertIsNone(self._gerrit_ref('refs/heads/cr159'))
        self.assertEqual(self._gerrit_ref('refs/heads/cr160'), self.fork)

    def test_refuses_a_chain_without_change_ids(self):
        self._git(self.dir, 'checkout', '-q', 'cr160')
        self._commit('No Change-Id')
        self._git(self.dir, 'push', '-q', 'origin', 'cr160')
        self._git(self.dir, 'fetch', '-q', 'origin')
        gerrit = _FakeGerrit(
            str(self.gerrit_dir), [Change(_change_id(3), 13, 1, 'NEW')]
        )

        status, lines = self._upload(gerrit)

        self.assertEqual(status, 1)
        self.assertIn('(No Change-Id) has no Change-Id.', '\n'.join(lines))
        self.assertIsNone(self._gerrit_ref('refs/for/cr160'))
        self.assertEqual(gerrit.abandoned, [])

    def test_stops_when_gerrit_diverged_from_the_base(self):
        self._git(self.dir, 'checkout', '-q', '-b', 'diverged', self.fork)
        diverged = self._commit('Only on Gerrit')
        self._git(
            self.dir,
            'push',
            '-q',
            str(self.gerrit_dir),
            f'{diverged}:refs/heads/master',
        )
        gerrit = _FakeGerrit(str(self.gerrit_dir), [])

        status, lines = self._upload(gerrit)

        self.assertEqual(status, 1)
        self.assertIn("Gerrit's master has diverged", '\n'.join(lines))
        self.assertEqual(self._gerrit_ref('refs/heads/master'), diverged)
        self.assertIsNone(self._gerrit_ref('refs/for/cr160'))

    def test_dry_run_changes_nothing(self):
        gerrit = _FakeGerrit(
            str(self.gerrit_dir),
            [
                Change(_change_id(2), 12, 3, 'ABANDONED'),
                Change(_change_id(3), 13, 2, 'NEW'),
            ],
        )

        status, lines = self._upload(gerrit, dry_run=True)

        self.assertEqual(status, 0, msg=lines)
        self.assertIsNone(self._gerrit_ref('refs/heads/master'))
        self.assertIsNone(self._gerrit_ref('refs/for/cr160'))
        self.assertEqual((gerrit.restored, gerrit.abandoned), ([], []))
        text = '\n'.join(lines)
        self.assertIn('Restoring 12', text)
        self.assertIn('Abandoning 13', text)


if __name__ == '__main__':
    unittest.main()
