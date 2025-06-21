#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path
import shutil
from datetime import datetime

import brockit
from brockit import ApplyPatchesRecord

from test.fake_chromium_src import FakeChromiumSrc


class BrockitTest(unittest.TestCase):
    """Test the patchfile generation and application."""

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.fake_chromium_src.add_dep('v8')
        self.fake_chromium_src.add_dep('third_party/test1')

        # Patch VERSION_UPGRADE_FILE to be under self.fake_chromium_src.brave
        brockit.VERSION_UPGRADE_FILE = (self.fake_chromium_src.brave /
                                        '.version_upgrade')

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

        # Ensure there is an empty line at the end of the file before the update
        self.assertTrue(test_pinslist_path.read_text().endswith('\n'),
                        "File does not end with an empty line before update.")

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

        # Ensure there is still an empty line at the end of the file after
        self.assertTrue(test_pinslist_path.read_text().endswith('\n'),
                        "File does not end with an empty line after update.")

    def test_requires_conflict_resolution(self):
        """Test ApplyPatchesRecord.requires_conflict_resolution"""

        # Case 1: No conflicts, no deleted files, no broken patches
        record = ApplyPatchesRecord()
        self.assertFalse(record.requires_conflict_resolution())

        # Case 2: Conflicts present
        record = ApplyPatchesRecord(files_with_conflicts=['file1'])
        self.assertTrue(record.requires_conflict_resolution())

        # Case 3: Deleted patches present
        record = ApplyPatchesRecord(patches_to_deleted_files=['patch1'])
        self.assertTrue(record.requires_conflict_resolution())

        # Case 4: Broken patches present
        record = ApplyPatchesRecord(broken_patches=['patch2'])
        self.assertTrue(record.requires_conflict_resolution())

        # Case 5: Multiple issues present
        record = ApplyPatchesRecord(files_with_conflicts=['file1'],
                                    patches_to_deleted_files=['patch1'],
                                    broken_patches=['patch2'])
        self.assertTrue(record.requires_conflict_resolution())

    def test_stage_all_patches(self):
        """Test ApplyPatchesRecord.stage_all_patches"""

        # Step 1: Commit files to fake repositories
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_file1.idl')
        test_file_v8 = Path('v8/test_file2.cc')
        test_file_third_party = Path('third_party/test1/test_file3.h')
        unrelated_file = Path(
            'chrome/common/extensions/api/unrelated_file.idl')

        # Write and commit files to respective repositories
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Initial content for Chromium file.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_file1.idl',
                                      self.fake_chromium_src.chromium)

        self.fake_chromium_src.write_and_stage_file(
            test_file_v8, 'Initial content for V8 file.',
            self.fake_chromium_src.chromium / 'v8')
        self.fake_chromium_src.commit('Add test_file2.cc',
                                      self.fake_chromium_src.chromium / 'v8')

        self.fake_chromium_src.write_and_stage_file(
            test_file_third_party, 'Initial content for third_party file.',
            self.fake_chromium_src.chromium / 'third_party/test1')
        self.fake_chromium_src.commit(
            'Add test_file3.h',
            self.fake_chromium_src.chromium / 'third_party/test1')

        # Add an unrelated file and commit it
        self.fake_chromium_src.write_and_stage_file(
            unrelated_file, 'Initial content for unrelated file.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add unrelated_file.idl',
                                      self.fake_chromium_src.chromium)

        # Step 2: Modify files directly using write_text
        (self.fake_chromium_src.chromium /
         test_file_chromium).write_text('Modified content for Chromium file.')
        (self.fake_chromium_src.chromium / 'v8' /
         test_file_v8).write_text('Modified content for V8 file.')
        (self.fake_chromium_src.chromium / 'third_party/test1' /
         test_file_third_party
         ).write_text('Modified content for third_party file.')
        (self.fake_chromium_src.chromium /
         unrelated_file).write_text('Modified content for unrelated file.')

        # Run update_patches to generate patches
        self.fake_chromium_src.run_update_patches()

        # Step 3: Create ApplyPatchesRecord and test stage_all_patches
        record = ApplyPatchesRecord(
            patch_files={
                self.fake_chromium_src.chromium: [
                    brockit.Patchfile(path=self.fake_chromium_src.
                                      get_patchfile_path_for_source(
                                          self.fake_chromium_src.chromium,
                                          test_file_chromium))
                ],
                self.fake_chromium_src.chromium / 'v8': [
                    brockit.Patchfile(path=self.fake_chromium_src.
                                      get_patchfile_path_for_source(
                                          self.fake_chromium_src.chromium /
                                          'v8', test_file_v8))
                ],
                self.fake_chromium_src.chromium / 'third_party/test1': [
                    brockit.Patchfile(
                        path=self.fake_chromium_src.
                        get_patchfile_path_for_source(
                            self.fake_chromium_src.chromium /
                            'third_party/test1', test_file_third_party))
                ],
            })

        # Stage all patches
        record.stage_all_patches()

        # Verify that the listed patches are staged
        staged_files = self.fake_chromium_src._run_git_command(
            ['diff', '--cached', '--name-only'], self.fake_chromium_src.brave)
        self.assertIn(
            Path(
                self.fake_chromium_src.get_patchfile_path_for_source(
                    self.fake_chromium_src.chromium,
                    test_file_chromium)).as_posix(), staged_files)
        self.assertIn(
            Path(
                self.fake_chromium_src.get_patchfile_path_for_source(
                    self.fake_chromium_src.chromium / 'v8',
                    test_file_v8)).as_posix(), staged_files)
        self.assertIn(
            Path(
                self.fake_chromium_src.get_patchfile_path_for_source(
                    self.fake_chromium_src.chromium / 'third_party/test1',
                    test_file_third_party)).as_posix(), staged_files)

        # Verify that unrelated patches are not staged
        self.assertNotIn(
            Path(
                self.fake_chromium_src.get_patchfile_path_for_source(
                    self.fake_chromium_src.chromium,
                    unrelated_file)).as_posix(), staged_files)

    def test_continuation_file_save_and_load(self):
        """Test saving and loading of ContinuationFile."""
        target_version = brockit.Version('135.0.7037.1')
        working_version = brockit.Version('134.0.7036.0')
        base_version = brockit.Version('134.0.7035.0')

        # Create a ContinuationFile instance and save it
        continuation = brockit.ContinuationFile(
            target_version=target_version,
            working_version=working_version,
            base_version=base_version,
            has_shown_advisory=True,
            apply_record=None)
        continuation.save()

        # Load the continuation file with check=True
        loaded_continuation = brockit.ContinuationFile.load(
            target_version=target_version,
            working_version=working_version,
            check=True)
        self.assertEqual(loaded_continuation.target_version, target_version)
        self.assertEqual(loaded_continuation.working_version, working_version)
        self.assertEqual(loaded_continuation.base_version, base_version)
        self.assertTrue(loaded_continuation.has_shown_advisory)

        # Load the continuation file with check=False
        loaded_continuation_no_check = brockit.ContinuationFile.load(
            target_version=target_version,
            working_version=working_version,
            check=False)
        self.assertEqual(loaded_continuation_no_check.target_version,
                         target_version)

    def test_continuation_file_load_nonexistent(self):
        """Test loading a nonexistent ContinuationFile."""
        target_version = brockit.Version('135.0.7037.1')
        working_version = brockit.Version('134.0.7036.0')

        # Attempt to load a nonexistent continuation file with check=False
        continuation = brockit.ContinuationFile.load(
            target_version=target_version,
            working_version=working_version,
            check=False)
        self.assertIsNone(continuation)

        # Attempt to load a nonexistent continuation file with check=True
        with self.assertRaises(FileNotFoundError):
            brockit.ContinuationFile.load(target_version=target_version,
                                          working_version=working_version,
                                          check=True)

    def test_continuation_file_load_version_mismatch(self):
        """Test loading a ContinuationFile with mismatched versions."""
        target_version = brockit.Version('135.0.7037.1')
        working_version = brockit.Version('134.0.7036.0')
        base_version = brockit.Version('134.0.7035.0')

        # Create and save a ContinuationFile with specific versions
        continuation = brockit.ContinuationFile(
            target_version=brockit.Version(
                '136.0.8000.0'),  # Different target version
            working_version=brockit.Version('134.0.7036.0'),
            base_version=base_version)
        continuation.save()

        # Attempt to load with mismatched target_version and check=False
        loaded_continuation = brockit.ContinuationFile.load(
            target_version=target_version,
            working_version=working_version,
            check=False)
        self.assertIsNone(loaded_continuation)

        # Attempt to load with mismatched target_version and check=True
        with self.assertRaises(TypeError):
            brockit.ContinuationFile.load(target_version=target_version,
                                          working_version=working_version,
                                          check=True)

    def test_continuation_file_load_working_version_mismatch(self):
        """Test loading a ContinuationFile with mismatched working versions."""
        target_version = brockit.Version('135.0.7037.1')
        working_version = brockit.Version('134.0.7036.0')
        base_version = brockit.Version('134.0.7035.0')

        # Create and save a ContinuationFile with specific versions
        continuation = brockit.ContinuationFile(
            target_version=target_version,
            working_version=brockit.Version(
                '134.0.5000.0'),  # Different working version
            base_version=base_version)
        continuation.save()

        # Attempt to load with mismatched working_version and check=False
        loaded_continuation = brockit.ContinuationFile.load(
            target_version=target_version,
            working_version=working_version,
            check=False)
        self.assertIsNone(loaded_continuation)

        # Attempt to load with mismatched working_version and check=True
        with self.assertRaises(TypeError):
            brockit.ContinuationFile.load(target_version=target_version,
                                          working_version=working_version,
                                          check=True)

    def test_continuation_file_clear(self):
        """Test clearing the ContinuationFile."""
        target_version = brockit.Version('135.0.7037.1')
        working_version = brockit.Version('134.0.7036.0')
        base_version = brockit.Version('134.0.7035.0')

        # Create and save a ContinuationFile
        continuation = brockit.ContinuationFile(
            target_version=target_version,
            working_version=working_version,
            base_version=base_version)
        continuation.save()

        # Verify the file exists
        self.assertTrue(brockit.VERSION_UPGRADE_FILE.exists())

        # Clear the continuation file
        brockit.ContinuationFile.clear()

        # Verify the file no longer exists
        self.assertFalse(brockit.VERSION_UPGRADE_FILE.exists())

        # Check double delete is noop
        brockit.ContinuationFile.clear()


if __name__ == "__main__":
    unittest.main()
