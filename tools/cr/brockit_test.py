#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import PurePath, Path
import shutil
from datetime import datetime, timedelta

import brockit

from test.fake_chromium_src import FakeChromiumSrc


class BrockitTest(unittest.TestCase):
    """Test the patchfile generation and application."""

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.fake_chromium_src.add_dep('v8')
        self.fake_chromium_src.add_dep('third_party/test1')
        self.addCleanup(self.fake_chromium_src.cleanup)

    def test_get_current_branch_upstream_name(self):
        """Test that the upstream branch name is correctly retrieved."""
        # Create a remote for the Brave repository
        self.fake_chromium_src.create_brave_remote()

        # Set the upstream branch for the current branch
        repo_path = self.fake_chromium_src.brave
        self.fake_chromium_src._run_git_command(
            ['checkout', '-b', 'test-branch'], repo_path)
        self.fake_chromium_src._run_git_command(
            ['push', '--set-upstream', 'origin', 'test-branch'], repo_path)

        # Verify the upstream branch name
        upstream_name = brockit._get_current_branch_upstream_name()
        self.assertEqual(upstream_name, 'origin/test-branch')

    def test_get_current_branch_upstream_name_no_upstream(self):
        """Test when there's no upstream branch."""
        # Create a branch without setting an upstream
        repo_path = self.fake_chromium_src.brave
        self.fake_chromium_src._run_git_command(
            ['checkout', '-b', 'no-upstream-branch'], repo_path)

        # Verify that _get_current_branch_upstream_name returns None
        upstream_name = brockit._get_current_branch_upstream_name()
        self.assertIsNone(upstream_name)

    def test_update_pinslist_timestamp(self):
        """Test that the pinslist timestamp is updated correctly."""
        # Copy the file from the Chromium source directory to the test repo
        original_pinslist_path = (
            Path(__file__).parent.parent.parent /
            'chromium_src/net/tools/transport_security_state_generator/'
            'input_file_parsers.cc')
        test_pinslist_path = (
            self.fake_chromium_src.brave /
            'chromium_src/net/tools/transport_security_state_generator/'
            'input_file_parsers.cc')

        test_pinslist_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(original_pinslist_path, test_pinslist_path)

        # Stage and commit the file to the Brave fake repository
        self.fake_chromium_src._run_git_command(
            ['add', str(test_pinslist_path)], self.fake_chromium_src.brave)
        self.fake_chromium_src._run_git_command(
            ['commit', '-m', 'Add pinslist file for testing'],
            self.fake_chromium_src.brave)

        # Call the function to update the timestamp
        before_update = datetime.now()
        readable_timestamp = brockit._update_pinslist_timestamp()

        # Verify that the timestamp is updated
        updated_content = test_pinslist_path.read_text()
        self.assertIn(f'# Last updated: {readable_timestamp}', updated_content)

        # Verify that the readable timestamp is within a valid range
        timestamp_datetime = datetime.strptime(readable_timestamp,
                                               '%a %b %d %H:%M:%S %Y')
        self.assertTrue(
            0 <= (before_update - timestamp_datetime).total_seconds() <= 10)


if __name__ == "__main__":
    unittest.main()
