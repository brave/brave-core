#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import subprocess
import unittest
from pathlib import Path

from test.fake_chromium_src import FakeChromiumSrc
from git_status import GitStatus


class GitStatusTest(unittest.TestCase):

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)

    # -- Helpers ---------------------------------------------------------------

    def _brave(self):
        return self.fake_chromium_src.brave

    def _commit_file(self, path, content='content'):
        self.fake_chromium_src.write_and_stage_file(path, content,
                                                    self._brave())
        self.fake_chromium_src.commit(f'Add {path}', self._brave())

    def _stage_file(self, path, content='content'):
        self.fake_chromium_src.write_and_stage_file(path, content,
                                                    self._brave())

    def _stage_delete(self, path):
        self.fake_chromium_src.delete_file(path, self._brave())

    def _write_file(self, path, content='content'):
        full = Path(self._brave() / path)
        full.parent.mkdir(parents=True, exist_ok=True)
        full.write_text(content)

    def _delete_file(self, path):
        Path(self._brave() / path).unlink()

    def _git_mv(self, old_path, new_path):
        subprocess.check_call(['git', 'mv', old_path, new_path],
                              cwd=self._brave(),
                              stderr=subprocess.DEVNULL)

    def _assert_empty(self, status):
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.renamed, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, [])

    # -- XY state tests --------------------------------------------------------

    def test_clean_state(self):
        """No changes: all areas are empty."""
        self._assert_empty(GitStatus())
        self.assertFalse(GitStatus().has_staged_files())
        self.assertFalse(GitStatus().has_deleted_patch_files())
        self.assertFalse(GitStatus().has_untracked_patch_files())

    def test_staged_added(self):
        """XY='A ': new files staged."""
        self._stage_file('a.txt')
        self._stage_file('b.txt')
        status = GitStatus()
        self.assertEqual(status.staged.added, ['a.txt', 'b.txt'])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, [])

    def test_staged_added_then_unstaged_modified(self):
        """XY='AM': files staged then modified."""
        self._stage_file('a.txt', 'v1')
        self._stage_file('b.txt', 'v1')
        self._write_file('a.txt', 'v2')
        self._write_file('b.txt', 'v2')
        status = GitStatus()
        self.assertEqual(status.staged.added, ['a.txt', 'b.txt'])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, ['a.txt', 'b.txt'])
        self.assertEqual(status.unstaged.deleted, [])

    def test_staged_added_then_unstaged_deleted(self):
        """XY='AD': files staged then deleted."""
        self._stage_file('a.txt')
        self._stage_file('b.txt')
        self._delete_file('a.txt')
        self._delete_file('b.txt')
        status = GitStatus()
        self.assertEqual(status.staged.added, ['a.txt', 'b.txt'])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, ['a.txt', 'b.txt'])

    def test_staged_modified(self):
        """XY='M ': committed files with staged modifications."""
        self._commit_file('a.txt', 'v1')
        self._commit_file('b.txt', 'v1')
        self._stage_file('a.txt', 'v2')
        self._stage_file('b.txt', 'v2')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, ['a.txt', 'b.txt'])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, [])

    def test_staged_modified_then_unstaged_modified(self):
        """XY='MM': staged modification, then further modified."""
        self._commit_file('a.txt', 'v1')
        self._commit_file('b.txt', 'v1')
        self._stage_file('a.txt', 'v2')
        self._stage_file('b.txt', 'v2')
        self._write_file('a.txt', 'v3')
        self._write_file('b.txt', 'v3')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, ['a.txt', 'b.txt'])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, ['a.txt', 'b.txt'])
        self.assertEqual(status.unstaged.deleted, [])

    def test_staged_modified_then_unstaged_deleted(self):
        """XY='MD': staged modification, then deleted."""
        self._commit_file('a.txt', 'v1')
        self._commit_file('b.txt', 'v1')
        self._stage_file('a.txt', 'v2')
        self._stage_file('b.txt', 'v2')
        self._delete_file('a.txt')
        self._delete_file('b.txt')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, ['a.txt', 'b.txt'])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, ['a.txt', 'b.txt'])

    def test_staged_deleted(self):
        """XY='D ': committed files with staged deletions."""
        self._commit_file('a.txt')
        self._commit_file('b.txt')
        self._stage_delete('a.txt')
        self._stage_delete('b.txt')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, ['a.txt', 'b.txt'])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, [])

    def test_unstaged_modified(self):
        """XY=' M': committed files modified without staging."""
        self._commit_file('a.txt', 'v1')
        self._commit_file('b.txt', 'v1')
        self._write_file('a.txt', 'v2')
        self._write_file('b.txt', 'v2')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, ['a.txt', 'b.txt'])
        self.assertEqual(status.unstaged.deleted, [])

    def test_unstaged_deleted(self):
        """XY=' D': committed files deleted without staging."""
        self._commit_file('a.txt')
        self._commit_file('b.txt')
        self._delete_file('a.txt')
        self._delete_file('b.txt')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, ['a.txt', 'b.txt'])

    def test_untracked(self):
        """XY='??': untracked files."""
        self._write_file('a.txt')
        self._write_file('b.txt')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.added, ['a.txt', 'b.txt'])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, [])

    def test_staged_renamed(self):
        """XY='R ': committed files renamed via git mv."""
        self._commit_file('a_old.txt')
        self._commit_file('b_old.txt')
        self._git_mv('a_old.txt', 'a_new.txt')
        self._git_mv('b_old.txt', 'b_new.txt')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.renamed, [('a_old.txt', 'a_new.txt'),
                                          ('b_old.txt', 'b_new.txt')])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, [])
        self.assertEqual(status.unstaged.deleted, [])

    def test_staged_renamed_then_unstaged_modified(self):
        """XY='RM': staged rename, then new path modified."""
        self._commit_file('a_old.txt', 'v1')
        self._commit_file('b_old.txt', 'v1')
        self._git_mv('a_old.txt', 'a_new.txt')
        self._git_mv('b_old.txt', 'b_new.txt')
        self._write_file('a_new.txt', 'v2')
        self._write_file('b_new.txt', 'v2')
        status = GitStatus()
        self.assertEqual(status.staged.added, [])
        self.assertEqual(status.staged.modified, [])
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.renamed, [('a_old.txt', 'a_new.txt'),
                                          ('b_old.txt', 'b_new.txt')])
        self.assertEqual(status.unstaged.added, [])
        self.assertEqual(status.unstaged.modified, ['a_new.txt', 'b_new.txt'])
        self.assertEqual(status.unstaged.deleted, [])

    # -- Helper method tests ---------------------------------------------------

    def test_has_staged_files(self):
        """has_staged_files is True for any staged change across all types."""
        self.assertFalse(GitStatus().has_staged_files())

        self._stage_file('a.txt')
        self._stage_file('b.txt')
        status = GitStatus()
        self.assertTrue(status.has_staged_files())
        self.assertEqual(len(status.staged.added), 2)

        self.fake_chromium_src.commit('add', self._brave())
        self._stage_file('a.txt', 'v2')
        self._stage_file('b.txt', 'v2')
        status = GitStatus()
        self.assertTrue(status.has_staged_files())
        self.assertEqual(len(status.staged.modified), 2)

        self.fake_chromium_src.commit('modify', self._brave())
        self._stage_delete('a.txt')
        self._stage_delete('b.txt')
        status = GitStatus()
        self.assertTrue(status.has_staged_files())
        self.assertEqual(len(status.staged.deleted), 2)

        self.fake_chromium_src.commit('delete', self._brave())
        self._commit_file('c.txt')
        self._commit_file('d.txt')
        self._git_mv('c.txt', 'c_new.txt')
        self._git_mv('d.txt', 'd_new.txt')
        status = GitStatus()
        self.assertTrue(status.has_staged_files())
        self.assertEqual(len(status.renamed), 2)

    def test_has_deleted_patch_files(self):
        """has_deleted_patch_files() detects staged/unstaged deletions."""
        self.assertFalse(GitStatus().has_deleted_patch_files())
        self.assertEqual(GitStatus().staged.deleted, [])
        self.assertEqual(GitStatus().unstaged.deleted, [])

        # Staged deletions of multiple patch files are detected.
        self._commit_file('patches/a.patch', 'patch a')
        self._commit_file('patches/b.patch', 'patch b')
        self._stage_delete('patches/a.patch')
        self._stage_delete('patches/b.patch')
        status = GitStatus()
        self.assertTrue(status.has_deleted_patch_files())
        self.assertEqual(status.staged.deleted,
                         ['patches/a.patch', 'patches/b.patch'])
        self.assertEqual(status.unstaged.deleted, [])

        # After committing the deletions, the slate is clean again.
        self.fake_chromium_src.commit('Delete patches', self._brave())
        self.assertFalse(GitStatus().has_deleted_patch_files())

        # Unstaged deletions of multiple patch files are also detected.
        self._commit_file('patches/a.patch', 'patch a again')
        self._commit_file('patches/b.patch', 'patch b again')
        self._delete_file('patches/a.patch')
        self._delete_file('patches/b.patch')
        status = GitStatus()
        self.assertTrue(status.has_deleted_patch_files())
        self.assertEqual(status.staged.deleted, [])
        self.assertEqual(status.unstaged.deleted,
                         ['patches/a.patch', 'patches/b.patch'])

    def test_has_untracked_patch_files(self):
        """has_untracked_patch_files() only fires for untracked patches."""
        self.assertFalse(GitStatus().has_untracked_patch_files())
        self.assertEqual(GitStatus().unstaged.added, [])

        # Staged patch files do not count as untracked.
        self._stage_file('patches/staged_a.patch', 'content a')
        self._stage_file('patches/staged_b.patch', 'content b')
        status = GitStatus()
        self.assertFalse(status.has_untracked_patch_files())
        self.assertEqual(status.unstaged.added, [])

        # Multiple untracked patch files are all detected.
        self._write_file('patches/untracked_a.patch', 'untracked a')
        self._write_file('patches/untracked_b.patch', 'untracked b')
        status = GitStatus()
        self.assertTrue(status.has_untracked_patch_files())
        self.assertEqual(
            status.unstaged.added,
            ['patches/untracked_a.patch', 'patches/untracked_b.patch'])


if __name__ == "__main__":
    unittest.main()
