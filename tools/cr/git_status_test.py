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
        self.assertFalse(GitStatus().has_deleted_patch_files())

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
        self.assertTrue(GitStatus().has_deleted_patch_files())

    def test_has_untracked_patch_files(self):
        """Test has_untracked_patch_files method."""
        self.assertFalse(GitStatus().has_untracked_patch_files())

        # Create a patch file but do not stage it
        Path(self.fake_chromium_src.brave /
             'test_untracked.patch').write_text('Untracked patch content')

        # Run GitStatus and verify the untracked patch file is detected
        self.assertTrue(GitStatus().has_untracked_patch_files())


if __name__ == "__main__":
    unittest.main()
