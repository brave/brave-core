#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

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

    def test_has_deleted_patch_files(self):
        """Test has_deleted_patch_files method."""
        status = GitStatus()
        self.assertFalse(status.has_deleted_patch_files())
        self.assertEqual(status.deleted, [])

        # Create a patch file and commit it
        patch_file = 'patches/test.patch'
        self.fake_chromium_src.write_and_stage_file(
            patch_file, 'Patch content', self.fake_chromium_src.brave)
        self.fake_chromium_src.commit('Add test.patch',
                                      self.fake_chromium_src.brave)

        # Delete the patch file
        self.fake_chromium_src.delete_file(patch_file,
                                           self.fake_chromium_src.brave)

        # Run GitStatus and verify the deleted patch file is detected
        status = GitStatus()
        self.assertTrue(status.has_deleted_patch_files())
        self.assertEqual(status.deleted, [patch_file])

    def test_test_has_staged_files(self):
        """Test staged files summary."""
        status = GitStatus()
        self.assertFalse(status.has_staged_files())
        self.assertEqual(status.staged, [])

        # Stage one file and leave another one untracked.
        staged_file = 'staged_file.txt'
        untracked_file = 'untracked_file.txt'
        self.fake_chromium_src.write_and_stage_file(
            staged_file, 'staged content', self.fake_chromium_src.brave)
        Path(self.fake_chromium_src.brave /
             untracked_file).write_text('untracked content')

        status = GitStatus()
        self.assertTrue(status.has_staged_files())
        self.assertEqual(status.staged, [staged_file])

    def test_staged(self):
        """Test staged files summary."""
        status = GitStatus()
        self.assertFalse(status.has_staged_files())
        self.assertEqual(status.staged, [])

        # Stage one file and leave another one untracked.
        staged_file = 'staged_file.txt'
        untracked_file = 'untracked_file.txt'
        self.fake_chromium_src.write_and_stage_file(
            staged_file, 'staged content', self.fake_chromium_src.brave)
        Path(self.fake_chromium_src.brave /
             untracked_file).write_text('untracked content')

        status = GitStatus()
        self.assertTrue(status.has_staged_files())
        self.assertEqual(status.staged, [staged_file])

    def test_staged(self):
        """Test staged files summary."""
        status = GitStatus()
        self.assertFalse(status.has_staged_files())
        self.assertEqual(status.staged, [])

        # Stage one file and leave another one untracked.
        staged_file = 'staged_file.txt'
        untracked_file = 'untracked_file.txt'
        self.fake_chromium_src.write_and_stage_file(
            staged_file, 'staged content', self.fake_chromium_src.brave)
        Path(self.fake_chromium_src.brave /
             untracked_file).write_text('untracked content')

        status = GitStatus()
        self.assertTrue(status.has_staged_files())
        self.assertEqual(status.staged, [staged_file])

    def test_has_untracked_patch_files(self):
        """Test has_untracked_patch_files method."""
        status = GitStatus()
        self.assertFalse(status.has_untracked_patch_files())
        self.assertEqual(status.untracked, [])

        # Stage a patch file; this should not count as untracked.
        self.fake_chromium_src.write_and_stage_file(
            'staged.patch', 'staged patch content',
            self.fake_chromium_src.brave)
        status = GitStatus()
        self.assertFalse(status.has_untracked_patch_files())
        self.assertEqual(status.untracked, [])

        # Stage a patch file; this should not count as untracked.
        self.fake_chromium_src.write_and_stage_file(
            'staged.patch', 'staged patch content',
            self.fake_chromium_src.brave)
        self.assertFalse(GitStatus().has_untracked_patch_files())

        # Stage a patch file; this should not count as untracked.
        self.fake_chromium_src.write_and_stage_file(
            'staged.patch', 'staged patch content',
            self.fake_chromium_src.brave)
        self.assertFalse(GitStatus().has_untracked_patch_files())

        # Create a patch file but do not stage it
        untracked_patch_file = 'test_untracked.patch'
        Path(self.fake_chromium_src.brave /
             untracked_patch_file).write_text('Untracked patch content')

        # Run GitStatus and verify the untracked patch file is detected
        status = GitStatus()
        self.assertTrue(status.has_untracked_patch_files())
        self.assertEqual(status.untracked, [untracked_patch_file])


if __name__ == "__main__":
    unittest.main()
