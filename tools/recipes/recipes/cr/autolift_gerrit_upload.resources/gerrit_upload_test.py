#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

import contextlib
import json
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

    def test_queries_are_read_page_by_page(self):
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
            changes = gerrit.open_changes('cr160')
        self.assertEqual(
            changes,
            [
                Change(_change_id(1), 1, 2, 'NEW'),
                Change(_change_id(2), 2, 2, 'NEW'),
            ],
        )
        self.assertIn('--start=1', ssh.call_args_list[1].args)
        self.assertIn(
            'project:project branch:master topic:cr160 status:open',
            ssh.call_args_list[0].args,
        )

    def test_abandoned_changes_are_found_by_change_id_in_chunks(self):
        """Whatever their topic: a CL can move between chains."""
        gerrit = gerrit_upload.Gerrit('ssh://gerrit.example.com/project')
        stats = '{"type": "stats", "moreChanges": false}\n'
        ids = [_change_id(n) for n in range(52)]
        with (
            mock.patch.object(gerrit_upload, 'CHANGE_IDS_PER_QUERY', 50),
            mock.patch.object(gerrit, '_ssh', return_value=stats) as ssh,
        ):
            self.assertEqual(gerrit.abandoned_changes(reversed(ids)), [])
        queries = [call.args[-1] for call in ssh.call_args_list]
        self.assertEqual(len(queries), 2)
        self.assertTrue(
            queries[0].startswith(
                'project:project branch:master status:abandoned '
                f'(change:{ids[0]} OR change:{ids[1]} OR'
            )
        )
        self.assertTrue(
            queries[1].endswith(f'(change:{ids[50]} OR change:{ids[51]})')
        )

    def test_known_change_ids_are_found_in_any_state(self):
        gerrit = gerrit_upload.Gerrit('ssh://gerrit.example.com/project')
        row = (
            f'{{"id": "{_change_id(1)}", "number": 1, "status": "MERGED", '
            '"currentPatchSet": {"number": 2}}\n'
            '{"type": "stats", "moreChanges": false}\n'
        )
        with mock.patch.object(gerrit, '_ssh', return_value=row) as ssh:
            known = gerrit.known_change_ids([_change_id(1), _change_id(2)])
        self.assertEqual(known, {_change_id(1)})
        self.assertEqual(
            ssh.call_args.args[-1],
            'project:project branch:master '
            f'(change:{_change_id(1)} OR change:{_change_id(2)})',
        )

    def test_patch_sets_are_queried_by_commit(self):
        gerrit = gerrit_upload.Gerrit('ssh://gerrit.example.com/project')
        stats = '{"type": "stats", "moreChanges": false}\n'
        with mock.patch.object(gerrit, '_ssh', return_value=stats) as ssh:
            self.assertFalse(gerrit.has_patch_set('a' * 40))
        self.assertIn(f'project:project commit:{"a" * 40}', ssh.call_args.args)

    _CL = 'https://gerrit.example.com/c/project/+/42'

    def _review_link(self, sha: str, patch_sets: list[tuple[int, str]]):
        gerrit = gerrit_upload.Gerrit('ssh://gerrit.example.com/project')
        row = json.dumps(
            {
                'id': _change_id(1),
                'number': 42,
                'url': self._CL,
                'patchSets': [
                    {'number': number, 'revision': revision}
                    for number, revision in patch_sets
                ],
            }
        )
        stats = '{"type": "stats", "moreChanges": false}\n'
        with mock.patch.object(
            gerrit, '_ssh', return_value=f'{row}\n{stats}'
        ) as ssh:
            found = gerrit.review_link(sha)
        self.assertIn('--patch-sets', ssh.call_args.args)
        return found

    def test_a_review_link_shows_what_changed_since_the_patch_set_before(self):
        self.assertEqual(
            self._review_link(
                'c' * 40, [(1, 'a' * 40), (2, 'b' * 40), (3, 'c' * 40)]
            ),
            (42, 3, f'{self._CL}/2..3'),
        )

    def test_a_new_cls_review_link_is_the_cl(self):
        self.assertEqual(
            self._review_link('a' * 40, [(1, 'a' * 40)]), (42, 1, self._CL)
        )

    def test_no_review_link_without_the_patch_set(self):
        self.assertIsNone(self._review_link('d' * 40, [(1, 'a' * 40)]))

    def test_review_links_skip_commits_on_no_cl(self):
        gerrit = gerrit_upload.Gerrit('ssh://gerrit.example.com/project')
        found = {'a' * 40: (42, 2, f'{self._CL}/1..2')}
        logged = []
        kept = {'sha': 'a' * 40, 'subject': 'Fix the build'}
        with mock.patch.object(gerrit, 'review_link', side_effect=found.get):
            links = gerrit_upload.review_links(
                gerrit,
                [kept, {'sha': 'b' * 40, 'subject': 'Gone'}],
                log=logged.append,
            )
        self.assertEqual(
            links,
            [
                {
                    'commit': kept,
                    'change': 42,
                    'patch_set': 2,
                    'url': f'{self._CL}/1..2',
                }
            ],
        )
        self.assertEqual(logged, [f'{"b" * 12} (Gone) is on no CL.'])


class _FakeGerrit(gerrit_upload.Gerrit):
    """A bare repository for the pushes, and lists for the CLs."""

    def __init__(
        self,
        url: str,
        open_changes: list[Change] = (),
        abandoned: list[Change] = (),
        patch_sets: set[str] = frozenset(),
    ):
        super().__init__(url)
        self._open = list(open_changes)
        self._abandoned = list(abandoned)
        self._patch_sets = set(patch_sets)
        self.abandoned: list[int] = []
        self.restored: list[int] = []

    def open_changes(self, topic: str) -> list[Change]:
        return self._open

    def abandoned_changes(self, change_ids) -> list[Change]:
        change_ids = set(change_ids)
        return [c for c in self._abandoned if c.change_id in change_ids]

    def known_change_ids(self, change_ids) -> set[str]:
        return set(change_ids) & {
            c.change_id for c in [*self._open, *self._abandoned]
        }

    def has_patch_set(self, sha: str) -> bool:
        return sha in self._patch_sets

    def abandon(self, change: Change, message: str) -> None:
        self.abandoned.append(change.number)

    def restore(self, change: Change, message: str) -> None:
        self.restored.append(change.number)


class UploadTest(unittest.TestCase):
    """`cr160`'s own commits on top of a base, as autolift leaves the
    checkout, uploaded to a bare repository standing in for Gerrit."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        stack.enter_context(
            mock.patch.dict(
                os.environ,
                {
                    'GIT_CONFIG_GLOBAL': os.devnull,
                    'GIT_CONFIG_NOSYSTEM': '1',
                    'GIT_AUTHOR_NAME': 'A',
                    'GIT_AUTHOR_EMAIL': 'a@b.c',
                    'GIT_COMMITTER_NAME': 'A',
                    'GIT_COMMITTER_EMAIL': 'a@b.c',
                },
            )
        )
        self.dir = self.root / 'brave'
        self.origin = self.root / 'origin.git'
        self.gerrit_dir = self.root / 'gerrit.git'
        for bare in (self.origin, self.gerrit_dir):
            self._git(self.root, 'init', '-q', '--bare', str(bare))
        # Gerrit takes the push options of `refs/for/` pushes.
        self._git(
            self.gerrit_dir, 'config', 'receive.advertisePushOptions', 'true'
        )
        self.dir.mkdir()
        self._git(self.dir, 'init', '-q', '-b', 'master')
        self._git(self.dir, 'remote', 'add', 'origin', str(self.origin))
        self.base = self._commit('Base')
        self._git(self.dir, 'checkout', '-q', '-b', 'cr160')
        self.chain = [
            self._commit(f'Fix {n}', change_id=_change_id(n)) for n in (1, 2)
        ]
        self._git(self.dir, 'push', '-q', 'origin', 'master', 'cr160')
        self._git(self.dir, 'fetch', '-q', 'origin')
        # The commits whose CLs the last `_upload` created.
        self.created: list[ChainCommit] = []

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

    def _mirror_master(self, sha: str) -> None:
        """What `gerrit/mirror_brave_core` does before the upload."""
        self._git(
            self.dir,
            'push',
            '-q',
            str(self.gerrit_dir),
            f'{sha}:refs/heads/master',
        )

    def _gerrit_ref(self, ref: str) -> str | None:
        output = self._git(
            self.gerrit_dir, 'for-each-ref', '--format=%(objectname)', ref
        )
        return output or None

    def _upload(
        self,
        gerrit: _FakeGerrit,
        dry_run: bool = False,
        start: str | None = None,
        ccs: tuple[str, ...] = (),
    ) -> tuple[int, str]:
        lines: list[str] = []
        self.created.clear()
        status = gerrit_upload.upload(
            cwd=self.dir,
            gerrit=gerrit,
            branch='cr160',
            start=start or self.base,
            head='origin/cr160',
            dry_run=dry_run,
            log=lines.append,
            created=self.created,
            ccs=ccs,
        )
        return status, '\n'.join(lines)

    def test_ccs_go_with_the_push(self):
        self._mirror_master(self.base)

        status, text = self._upload(
            _FakeGerrit(str(self.gerrit_dir)), ccs=('cdesouza', 'erogul')
        )

        self.assertEqual(status, 0, msg=text)
        self.assertEqual(self._gerrit_ref('refs/for/master'), self.chain[-1])
        _, text = self._upload(
            _FakeGerrit(str(self.gerrit_dir)),
            dry_run=True,
            ccs=('cdesouza', 'erogul'),
        )
        self.assertIn('-o cc=cdesouza -o cc=erogul', text)

    def test_reports_the_cls_the_push_creates(self):
        """Those whose Change-Id Gerrit had no CL of, in any state."""
        self._mirror_master(self.base)
        gerrit = _FakeGerrit(
            str(self.gerrit_dir),
            abandoned=[Change(_change_id(1), 11, 2, 'ABANDONED')],
        )

        status, text = self._upload(gerrit)

        self.assertEqual(status, 0, msg=text)
        self.assertEqual([c.sha for c in self.created], [self.chain[1]])

    def test_a_dry_run_says_which_cls_it_would_create(self):
        self._mirror_master(self.base)

        status, text = self._upload(
            _FakeGerrit(str(self.gerrit_dir)), dry_run=True
        )

        self.assertEqual(status, 0, msg=text)
        self.assertEqual(self.created, [])
        self.assertIn(f'Would create a CL for {self.chain[0][:12]}', text)

    def test_uploads_the_chain_for_master_with_its_topic(self):
        self._mirror_master(self.base)
        gerrit = _FakeGerrit(
            str(self.gerrit_dir),
            open_changes=[
                Change(_change_id(1), 11, 1, 'NEW'),
                Change(_change_id(3), 13, 2, 'NEW'),
            ],
            abandoned=[
                Change(_change_id(2), 12, 3, 'ABANDONED'),
                Change(_change_id(4), 14, 1, 'ABANDONED'),
            ],
        )

        status, text = self._upload(gerrit)

        self.assertEqual(status, 0, msg=text)
        self.assertEqual(self._gerrit_ref('refs/for/master'), self.chain[-1])
        # Only the mirror moves Gerrit's branches.
        self.assertEqual(self._gerrit_ref('refs/heads/master'), self.base)
        self.assertIsNone(self._gerrit_ref('refs/heads/cr160'))
        # Back on the branch, and left it.
        self.assertEqual(gerrit.restored, [12])
        self.assertEqual(gerrit.abandoned, [13])

    def test_a_cl_moves_with_its_commit_to_this_chain(self):
        """A commit picked from `cr159` keeps its Change-Id, and its CL:
        not one of the topic's CLs yet, so the push updates it, and moves it
        to the topic. It is restored first if `cr159` had abandoned it."""
        self._mirror_master(self.base)
        gerrit = _FakeGerrit(
            str(self.gerrit_dir),
            abandoned=[Change(_change_id(2), 22, 4, 'ABANDONED')],
        )

        status, text = self._upload(gerrit)

        self.assertEqual(status, 0, msg=text)
        self.assertEqual(self._gerrit_ref('refs/for/master'), self.chain[-1])
        self.assertEqual(gerrit.restored, [22])
        self.assertEqual(gerrit.abandoned, [])

    def test_a_base_gerrit_has_as_patch_sets_is_not_uploaded_again(self):
        """`cr160` on `cr159`, whose chain Gerrit has as CLs."""
        self._mirror_master(self.base)
        self._git(self.dir, 'checkout', '-q', '-b', 'cr159', self.base)
        cr159 = self._commit('cr159 fix', change_id=_change_id(9))
        self._git(self.dir, 'checkout', '-q', 'cr160')
        self._git(self.dir, 'rebase', '-q', cr159)
        self._git(self.dir, 'push', '-q', '-f', 'origin', 'cr160')
        self._git(self.dir, 'fetch', '-q', 'origin')
        gerrit = _FakeGerrit(str(self.gerrit_dir), patch_sets={cr159})

        status, text = self._upload(gerrit, start=cr159)

        self.assertEqual(status, 0, msg=text)
        self.assertIn('2 commits on', text)

    def test_follows_a_gerrit_master_ahead_of_the_checkout(self):
        """The mirror may have brought commits the checkout hasn't fetched."""
        other = self.root / 'other'
        self._git(self.root, 'clone', '-q', str(self.origin), str(other))
        self._git(other, 'commit', '-q', '--allow-empty', '-m', 'Newer')
        self._git(
            other, 'push', '-q', str(self.gerrit_dir), 'HEAD:refs/heads/master'
        )
        gerrit = _FakeGerrit(str(self.gerrit_dir))

        status, text = self._upload(gerrit)

        self.assertEqual(status, 0, msg=text)
        self.assertEqual(self._gerrit_ref('refs/for/master'), self.chain[-1])

    def test_refuses_a_start_gerrit_does_not_have(self):
        gerrit = _FakeGerrit(str(self.gerrit_dir))

        status, text = self._upload(gerrit)

        self.assertEqual(status, 1)
        self.assertIn('Upload the base first.', text)
        self.assertIsNone(self._gerrit_ref('refs/for/master'))

    def test_refuses_a_start_off_the_branch(self):
        self._mirror_master(self.base)
        self._git(self.dir, 'checkout', '-q', '-b', 'elsewhere', self.base)
        elsewhere = self._commit('Elsewhere')

        status, text = self._upload(
            _FakeGerrit(str(self.gerrit_dir)), start=elsewhere
        )

        self.assertEqual(status, 1)
        self.assertIn('is not on origin/cr160', text)

    def test_refuses_a_chain_without_change_ids(self):
        self._mirror_master(self.base)
        self._git(self.dir, 'checkout', '-q', 'cr160')
        self._commit('No Change-Id')
        self._git(self.dir, 'push', '-q', 'origin', 'cr160')
        self._git(self.dir, 'fetch', '-q', 'origin')
        gerrit = _FakeGerrit(
            str(self.gerrit_dir), [Change(_change_id(3), 13, 1, 'NEW')]
        )

        status, text = self._upload(gerrit)

        self.assertEqual(status, 1)
        self.assertIn('(No Change-Id) has no Change-Id.', text)
        self.assertIsNone(self._gerrit_ref('refs/for/master'))
        self.assertEqual(gerrit.abandoned, [])

    def test_dry_run_changes_nothing(self):
        self._mirror_master(self.base)
        gerrit = _FakeGerrit(
            str(self.gerrit_dir),
            open_changes=[Change(_change_id(3), 13, 2, 'NEW')],
            abandoned=[Change(_change_id(2), 12, 3, 'ABANDONED')],
        )

        status, text = self._upload(gerrit, dry_run=True)

        self.assertEqual(status, 0, msg=text)
        self.assertIsNone(self._gerrit_ref('refs/for/master'))
        self.assertEqual((gerrit.restored, gerrit.abandoned), ([], []))
        self.assertIn('Restoring 12', text)
        self.assertIn('Abandoning 13', text)
        self.assertIn(':refs/for/master', text)


if __name__ == '__main__':
    unittest.main()
