#!/usr/bin/env vpython3
# # Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from pathlib import PurePath
import unittest

import repository
from repository import Repository

from test.fake_chromium_src import FakeChromiumSrc
from unittest.mock import patch


class RepositoryTest(unittest.TestCase):

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.fake_chromium_src.add_dep('v8')
        self.fake_chromium_src.add_dep('third_party/test1')
        self.addCleanup(self.fake_chromium_src.cleanup)

    def test_chromium_repository(self):
        self.assertEqual(repository.chromium.path,
                         PurePath(self.fake_chromium_src.chromium))
        self.assertTrue(repository.chromium.is_chromium)
        self.assertFalse(repository.chromium.is_brave)

    def test_brave_repository(self):
        self.assertEqual(repository.brave.path,
                         PurePath(self.fake_chromium_src.brave))
        self.assertTrue(repository.brave.is_brave)
        self.assertFalse(repository.brave.is_chromium)

    def test_to_brave(self):
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium).to_brave(),
            PurePath('brave'))
        self.assertEqual(
            Repository(self.fake_chromium_src.brave).to_brave(),
            PurePath('../brave'))
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium / 'v8').to_brave(),
            PurePath('../brave'))
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium /
                       'third_party/test1').to_brave(),
            PurePath('../../brave'))

    def test_from_brave(self):
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium).from_brave(),
            PurePath('../'))
        self.assertEqual(
            Repository(self.fake_chromium_src.brave).from_brave(),
            PurePath('../brave'))
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium / 'v8').from_brave(),
            PurePath('../v8'))
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium /
                       'third_party/test1').from_brave(),
            PurePath('../third_party/test1'))
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium /
                       'third_party/test1').from_brave('test_file.txt'),
            PurePath('../third_party/test1/test_file.txt'))

    def test_run_git(self):
        """Test Repository.run_git by verifying the hash of the last commit."""
        # Verify the hash of the last commit using Repository.run_git
        self.assertEqual(
            self.fake_chromium_src.commit_empty("Empty commit in brave",
                                                self.fake_chromium_src.brave),
            repository.brave.run_git("rev-parse", "HEAD"),
        )
        self.assertEqual(
            self.fake_chromium_src.commit_empty(
                "Empty commit in chromium", self.fake_chromium_src.chromium),
            repository.chromium.run_git("rev-parse", "HEAD"),
        )
        self.assertEqual(
            self.fake_chromium_src.commit_empty(
                "Empty commit in v8", self.fake_chromium_src.chromium / "v8"),
            Repository(self.fake_chromium_src.chromium / "v8").run_git(
                "rev-parse", "HEAD"),
        )
        self.assertEqual(
            self.fake_chromium_src.commit_empty(
                "Empty commit in third_party/test1",
                self.fake_chromium_src.chromium / "third_party/test1"),
            Repository(self.fake_chromium_src.chromium /
                       "third_party/test1").run_git("rev-parse", "HEAD"),
        )

    def test_run_git_no_trim(self):
        """Test that no_trim is correctly passed to run_git."""
        with patch("terminal.terminal.run_git") as mock_run_git:
            # Call run_git with no_trim=True for brave repository
            repository.brave.run_git("log", no_trim=True)
            mock_run_git.assert_called_once_with("log", no_trim=True)

            # Reset mock and call run_git with no_trim=False
            mock_run_git.reset_mock()
            repository.brave.run_git("log", no_trim=False)
            mock_run_git.assert_called_once_with("log", no_trim=False)

    def test_unstage_all_changes(self):
        """Test unstage_all_changes and has_staged_changed."""
        # Create a new file and stage it
        test_file = self.fake_chromium_src.chromium / "test_file.txt"
        test_file.write_text("Test content")
        repository.chromium.run_git("add", str(test_file))

        # Verify the file is staged and has_staged_changed returns True
        staged_files = repository.chromium.run_git("diff", "--cached",
                                                   "--name-only")
        self.assertIn("test_file.txt", staged_files)
        self.assertTrue(repository.chromium.has_staged_changed())

        # Unstage all changes
        repository.chromium.unstage_all_changes()

        # Verify the file is no longer staged and has_staged_changed is false
        staged_files_after = repository.chromium.run_git(
            "diff", "--cached", "--name-only")
        self.assertNotIn("test_file.txt", staged_files_after)
        self.assertFalse(repository.chromium.has_staged_changed())

    def test_get_commit_short_description(self):
        """Test get_commit_short_description using the chromium repository."""
        # Create an empty commit with a specific message
        commit_message = "Test commit message"
        commit_hash = self.fake_chromium_src.commit_empty(
            commit_message, self.fake_chromium_src.chromium)

        # Verify the short description of the commit
        short_description = repository.chromium.get_commit_short_description(
            commit_hash)
        self.assertEqual(short_description, commit_message)

    def test_git_commit(self):
        """Test git_commit."""

        # Case 1: Commit with staged changes
        test_file = self.fake_chromium_src.chromium / "test_file.txt"
        test_file.write_text("Test content")
        repository.chromium.run_git("add", str(test_file))
        commit_message = "Add test_file.txt"
        repository.chromium.git_commit(commit_message)

        # Verify the commit message
        last_commit_message = repository.chromium.get_commit_short_description(
        )
        self.assertEqual(last_commit_message, commit_message)

        # Case 2: Commit with no staged changes
        repository.chromium.git_commit("This should not create a commit")
        # Verify that the last commit message remains unchanged
        last_commit_message_after = (
            repository.chromium.get_commit_short_description())
        self.assertEqual(last_commit_message_after, commit_message)

    def test_is_valid_git_reference(self):
        """Test is_valid_git_reference using the chromium repository."""

        # Create a valid commit and verify the reference
        commit_message = "Valid commit"
        valid_commit_hash = self.fake_chromium_src.commit_empty(
            commit_message, self.fake_chromium_src.chromium)
        self.assertTrue(
            repository.chromium.is_valid_git_reference(valid_commit_hash))

        # Test an invalid reference
        self.assertFalse(
            repository.chromium.is_valid_git_reference("invalid_reference"))

    def test_last_changed(self):
        """Test last_changed using the chromium repository."""
        # Create and commit a file
        self.fake_chromium_src.commit_empty('empty #1',
                                            self.fake_chromium_src.chromium)
        file_commits = []
        self.fake_chromium_src.write_and_stage_file(
            "test_file.txt", "Initial content",
            self.fake_chromium_src.chromium)
        file_commits.append(
            self.fake_chromium_src.commit("Initial commit for test_file.txt",
                                          self.fake_chromium_src.chromium))
        self.fake_chromium_src.commit_empty('empty #2',
                                            self.fake_chromium_src.chromium)

        # Modify and commit the file again
        self.fake_chromium_src.commit_empty('empty #3',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.write_and_stage_file(
            "test_file.txt", "Updated content",
            self.fake_chromium_src.chromium)
        file_commits.append(
            self.fake_chromium_src.commit("Updated test_file.txt",
                                          self.fake_chromium_src.chromium))
        self.fake_chromium_src.commit_empty('empty #4',
                                            self.fake_chromium_src.chromium)

        # Verify last_changed returns the latest commit hash for the file
        last_changed_hash = repository.chromium.last_changed("test_file.txt")
        self.assertEqual(last_changed_hash[:7], file_commits[-1][:7])

        # Verify last_changed with a specific from_commit returns the correct
        # hash
        last_changed_from_initial = repository.chromium.last_changed(
            "test_file.txt", from_commit=f'{file_commits[-1]}~1')
        self.assertEqual(last_changed_from_initial[:7], file_commits[-2][:7])

    def test_read_file(self):
        """Test read_file, including reading multiple files."""
        # Create and commit two files
        self.fake_chromium_src.write_and_stage_file(
            "file1.txt", "Content of file 1\n",
            self.fake_chromium_src.chromium)
        commit1 = self.fake_chromium_src.commit(
            "Add file1.txt", self.fake_chromium_src.chromium)

        self.fake_chromium_src.write_and_stage_file(
            "file2.txt", "Content of file 2\n",
            self.fake_chromium_src.chromium)
        commit2 = self.fake_chromium_src.commit(
            "Add file2.txt", self.fake_chromium_src.chromium)

        # Test reading a single file
        content_file1 = repository.chromium.read_file("file1.txt",
                                                      commit=commit1)
        self.assertEqual(content_file1, "Content of file 1\n")

        # Test reading multiple files
        content_files = repository.chromium.read_file("file1.txt",
                                                      "file2.txt",
                                                      commit=commit2)
        self.assertIn("Content of file 1\n", content_files)
        self.assertIn("Content of file 2\n", content_files)

        # Test enforcing no_trim (ensure trailing newlines are preserved)
        self.fake_chromium_src.write_and_stage_file(
            "file3.txt", "Content with trailing newline\n\n",
            self.fake_chromium_src.chromium)
        commit3 = self.fake_chromium_src.commit(
            "Add file3.txt", self.fake_chromium_src.chromium)
        content_file3 = repository.chromium.read_file("file3.txt",
                                                      commit=commit3)
        self.assertEqual(content_file3, "Content with trailing newline\n\n")

    def test_current_branch(self):
        """Test current_branch using the chromium repository."""
        # Verify the default branch is "master" or "main"
        current_branch = repository.chromium.current_branch()
        self.assertIn(current_branch, ["master", "main"])

        # Create and switch to a new branch
        new_branch = "test-branch"
        repository.chromium.run_git("checkout", "-b", new_branch)

        # Verify the current branch is updated
        self.assertEqual(repository.chromium.current_branch(), new_branch)

    def test_relative_to_chromium(self):
        # Chromium root should be relative to itself as ''
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium).relative_to_chromium.
            as_posix(), '.')
        # Brave root should be relative to chromium as 'brave'
        self.assertEqual(
            Repository(
                self.fake_chromium_src.brave).relative_to_chromium.as_posix(),
            'brave')
        # Subdirectory v8 should be relative as 'v8'
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium /
                       'v8').relative_to_chromium.as_posix(), 'v8')
        # Nested subdirectory should be relative as 'third_party/test1'
        self.assertEqual(
            Repository(self.fake_chromium_src.chromium /
                       'third_party/test1').relative_to_chromium.as_posix(),
            'third_party/test1')


if __name__ == "__main__":
    unittest.main()
