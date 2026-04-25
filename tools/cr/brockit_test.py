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

    ############################################################################
    #### Test Versioned class

    def test_versioned_basic_initialisation(self):
        """Test Versioned with explicit target_version provided."""
        base_version = brockit.Version('134.0.7035.0')
        target_version = brockit.Version('135.0.7037.1')

        versioned = brockit.Versioned(base_version=base_version,
                                      target_version=target_version)

        self.assertEqual(versioned.base_version, base_version)
        self.assertEqual(versioned.target_version, target_version)

    def test_versioned_without_target_version(self):
        """Test Versioned without target_version (defaults to HEAD)."""
        # Set up a specific version in the test repository
        test_version = "135.0.7037.1"
        self.fake_chromium_src.update_brave_version(test_version)

        base_version = brockit.Version('134.0.7035.0')

        versioned = brockit.Versioned(base_version=base_version)

        self.assertEqual(versioned.base_version, base_version)
        self.assertEqual(str(versioned.target_version), test_version)

    def test_versioned_base_same_as_target(self):
        """Test Versioned with base version same as target."""
        base_version = brockit.Version('134.0.7035.0')
        target_version = brockit.Version('134.0.7035.0')

        with self.assertRaises(brockit.InvalidInputException) as context:
            brockit.Versioned(base_version=base_version,
                              target_version=target_version)

        self.assertIn(
            ('Target version 134.0.7035.0 is not higher than base version '
             '134.0.7035.0'), str(context.exception))

    def test_versioned_target_lower_than_base(self):
        """Test Versioned target lower than base."""
        base_version = brockit.Version('135.0.7037.1')
        target_version = brockit.Version('134.0.7035.0')  # Lower than base

        with self.assertRaises(brockit.InvalidInputException) as context:
            brockit.Versioned(base_version=base_version,
                              target_version=target_version)

        self.assertIn(
            ('Target version 134.0.7035.0 is not higher than base version '
             '135.0.7037.1'), str(context.exception))

    def test_versioned_head_target_version_lower_than_base(self):
        """Test Versioned with None target_version validates against HEAD."""
        # Set up a version in HEAD that's lower than base - should raise
        # exception
        head_version = "133.0.7000.0"
        self.fake_chromium_src.update_brave_version(head_version)

        base_version = brockit.Version('134.0.7035.0')  # Higher than HEAD

        with self.assertRaises(brockit.InvalidInputException) as context:
            brockit.Versioned(base_version=base_version)

        self.assertIn(
            ('Target version 133.0.7000.0 is not higher than base version '
             '134.0.7035.0'), str(context.exception))

    def test_versioned_save_updated_patches(self):
        """Test Versioned._save_updated_patches method."""
        # Set up versions
        base_version = brockit.Version('134.0.7035.0')
        target_version = brockit.Version('135.0.7037.1')

        # Create a Versioned instance
        versioned = brockit.Versioned(base_version=base_version,
                                      target_version=target_version)

        # Create some patch files in the patches directory
        patch1_path = self.fake_chromium_src.brave_patches / 'test1.patch'
        patch2_path = self.fake_chromium_src.brave_patches / 'test2.patch'
        patch3_path = (self.fake_chromium_src.brave_patches / 'v8' /
                       'test3.patch')

        # Create patch files with content
        patch1_path.write_text('patch content 1')
        patch2_path.write_text('patch content 2')
        patch3_path.parent.mkdir(parents=True, exist_ok=True)
        patch3_path.write_text('patch content 3')

        # Stage and commit the initial patches
        self.fake_chromium_src._run_git_command(
            ['add', str(patch1_path)], self.fake_chromium_src.brave)
        self.fake_chromium_src._run_git_command(
            ['add', str(patch2_path)], self.fake_chromium_src.brave)
        self.fake_chromium_src._run_git_command(
            ['add', str(patch3_path)], self.fake_chromium_src.brave)
        self.fake_chromium_src.commit('Add initial patches',
                                      self.fake_chromium_src.brave)

        # Modify the patch files (this simulates updated patches)
        patch1_path.write_text('modified patch content 1')
        patch2_path.write_text('modified patch content 2')
        patch3_path.write_text('modified patch content 3')

        # Add a non-patch file to ensure it's not included
        non_patch_path = (self.fake_chromium_src.brave_patches /
                          'not_a_patch.txt')
        non_patch_path.write_text('not a patch file')

        # Call _save_updated_patches
        versioned._save_updated_patches()

        # Verify the commit was created with the correct message
        log_output = self.fake_chromium_src._run_git_command(
            ['log', '-1', '--pretty=format:%s'], self.fake_chromium_src.brave)
        expected_message = (f'Update patches from Chromium {base_version} to '
                            f'Chromium {target_version}.')
        self.assertEqual(log_output, expected_message)

        # Verify that the patch files were staged and committed
        # Check that there are no unstaged changes for patch files
        diff_output = self.fake_chromium_src._run_git_command(
            ['diff', '--name-only', '*.patch'], self.fake_chromium_src.brave)
        self.assertEqual(diff_output, '')

        # Verify that the non-patch file was not staged
        status_output = self.fake_chromium_src._run_git_command(
            ['status', '--porcelain'], self.fake_chromium_src.brave)
        self.assertIn('?? patches/not_a_patch.txt', status_output)

    def test_versioned_save_updated_patches_no_changes_to_commit(self):
        """Test _save_updated_patches with nothing to commit."""
        # Set up versions
        base_version = brockit.Version('134.0.7035.0')
        target_version = brockit.Version('135.0.7037.1')

        # Create a Versioned instance
        versioned = brockit.Versioned(base_version=base_version,
                                      target_version=target_version)

        # Adding a single patch because otherwise the command to add *.patch
        # files errors out as git doesn't see a single patch file in the repo.
        # This should have no effect overall.
        patchfile = self.fake_chromium_src.brave_patches / 'test1.patch'
        patchfile.write_text('test patch content')
        self.fake_chromium_src._run_git_command(['add', str(patchfile)],
                                                self.fake_chromium_src.brave)
        self.fake_chromium_src.commit('Add initial patches',
                                      self.fake_chromium_src.brave)

        last_commit_log = self.fake_chromium_src._run_git_command(
            ['log', '-1', '--pretty=format:%s'], self.fake_chromium_src.brave)

        # Call _save_updated_patches with nothing should have no effect to the
        # repo.
        versioned._save_updated_patches()
        self.assertEqual(
            last_commit_log,
            self.fake_chromium_src._run_git_command(
                ['log', '-1', '--pretty=format:%s'],
                self.fake_chromium_src.brave))

        untracked_patch1 = (self.fake_chromium_src.brave_patches /
                            'untracked1.patch')
        untracked_patch2 = (self.fake_chromium_src.brave_patches / 'v8' /
                            'untracked2.patch')

        untracked_patch1.write_text('untracked patch content 1')
        untracked_patch2.parent.mkdir(parents=True, exist_ok=True)
        untracked_patch2.write_text('untracked patch content 2')

        # Untracked patch files should have no effect when calling
        # `_save_updated_patches`.
        versioned._save_updated_patches()
        self.assertEqual(
            last_commit_log,
            self.fake_chromium_src._run_git_command(
                ['log', '-1', '--pretty=format:%s'],
                self.fake_chromium_src.brave))

        non_patch_file = self.fake_chromium_src.brave / 'foo.txt'
        non_patch_file.write_text('not a patch file')

        # Untracked non-patch file should have no effect when calling
        # `_save_updated_patches`.
        versioned._save_updated_patches()
        self.assertEqual(
            last_commit_log,
            self.fake_chromium_src._run_git_command(
                ['log', '-1', '--pretty=format:%s'],
                self.fake_chromium_src.brave))

    def test_versioned_save_rebased_l10n_no_changes_to_commit(self):
        """Test _save_rebased_l10n with nothing to commit."""
        # Set up versions
        base_version = brockit.Version('134.0.7035.0')
        target_version = brockit.Version('135.0.7037.1')

        # Create a Versioned instance
        versioned = brockit.Versioned(base_version=base_version,
                                      target_version=target_version)

        # Adding l10n files because otherwise the command to add *.grd, *.grdp,
        #  *.xtb files errors out as git doesn't see any l10n files in the repo.
        # This should have no effect overall.
        grd_file = self.fake_chromium_src.brave / 'test_strings.grd'
        grdp_file = self.fake_chromium_src.brave / 'test_strings.grdp'
        xtb_file = self.fake_chromium_src.brave / 'test_strings.xtb'

        grd_file.write_text('<grd>test l10n content</grd>')
        grdp_file.write_text('<grdp>test l10n content</grdp>')
        xtb_file.write_text(
            '<?xml version="1.0" ?><translationbundle></translationbundle>')

        self.fake_chromium_src._run_git_command(['add', str(grd_file)],
                                                self.fake_chromium_src.brave)
        self.fake_chromium_src._run_git_command(['add', str(grdp_file)],
                                                self.fake_chromium_src.brave)
        self.fake_chromium_src._run_git_command(['add', str(xtb_file)],
                                                self.fake_chromium_src.brave)
        self.fake_chromium_src.commit('Add initial l10n files',
                                      self.fake_chromium_src.brave)

        last_commit_log = self.fake_chromium_src._run_git_command(
            ['log', '-1', '--pretty=format:%s'], self.fake_chromium_src.brave)

        # Call _save_rebased_l10n with nothing should have no effect to the
        # repo.
        versioned._save_rebased_l10n()
        self.assertEqual(
            last_commit_log,
            self.fake_chromium_src._run_git_command(
                ['log', '-1', '--pretty=format:%s'],
                self.fake_chromium_src.brave))

        # Create untracked non-l10n files
        non_l10n_file = self.fake_chromium_src.brave / 'foo.txt'
        non_l10n_file.write_text('not an l10n file')

        # Untracked non-l10n file should have no effect when calling
        # `_save_rebased_l10n`.
        versioned._save_rebased_l10n()
        self.assertEqual(
            last_commit_log,
            self.fake_chromium_src._run_git_command(
                ['log', '-1', '--pretty=format:%s'],
                self.fake_chromium_src.brave))

    def test_versioned_save_rebased_l10n(self):
        """Test Versioned._save_rebased_l10n method."""
        # Set up versions
        base_version = brockit.Version('134.0.7035.0')
        target_version = brockit.Version('135.0.7037.1')

        # Create a Versioned instance
        versioned = brockit.Versioned(base_version=base_version,
                                      target_version=target_version)

        # Create some tracked l10n files and commit them
        grd_file = self.fake_chromium_src.brave / 'test_strings.grd'
        grdp_file = self.fake_chromium_src.brave / 'test_strings.grdp'
        xtb_file = self.fake_chromium_src.brave / 'test_strings.xtb'

        grd_file.write_text('<grd>initial grd content</grd>')
        grdp_file.write_text('<grdp>initial grdp content</grdp>')
        xtb_file.write_text(
            '<?xml version="1.0" ?><translationbundle></translationbundle>')

        self.fake_chromium_src._run_git_command(['add', str(grd_file)],
                                                self.fake_chromium_src.brave)
        self.fake_chromium_src._run_git_command(['add', str(grdp_file)],
                                                self.fake_chromium_src.brave)
        self.fake_chromium_src._run_git_command(['add', str(xtb_file)],
                                                self.fake_chromium_src.brave)
        self.fake_chromium_src.commit('Add initial l10n files',
                                      self.fake_chromium_src.brave)

        # Modify the tracked l10n files
        grd_file.write_text('<grd>modified grd content</grd>')
        grdp_file.write_text('<grdp>modified grdp content</grdp>')
        xtb_file.write_text(
            '<?xml version="1.0" ?><translationbundle><translation id="test">'
            'modified</translation></translationbundle>')

        # Create untracked l10n files (these should also be staged since git
        #  add uses patterns, not -u)
        untracked_grd = self.fake_chromium_src.brave / 'untracked.grd'
        untracked_grdp = self.fake_chromium_src.brave / 'untracked.grdp'
        untracked_xtb = self.fake_chromium_src.brave / 'untracked.xtb'

        untracked_grd.write_text('<grd>untracked grd content</grd>')
        untracked_grdp.write_text('<grdp>untracked grdp content</grdp>')
        untracked_xtb.write_text(
            '<?xml version="1.0" ?><translationbundle><translation '
            'id="untracked">new</translation></translationbundle>')

        # Add a non-l10n file to ensure it's not included
        non_l10n_file = self.fake_chromium_src.brave / 'not_l10n.txt'
        non_l10n_file.write_text('not an l10n file')

        # Call _save_rebased_l10n
        versioned._save_rebased_l10n()

        # Verify the commit was created with the correct message
        log_output = self.fake_chromium_src._run_git_command(
            ['log', '-1', '--pretty=format:%s'], self.fake_chromium_src.brave)
        expected_message = f'Updated strings for Chromium {target_version}.'
        self.assertEqual(log_output, expected_message)

        # Verify that both tracked and untracked l10n files were staged and
        # committed. Check that there are no unstaged changes for l10n files.
        status_output = self.fake_chromium_src._run_git_command(
            ['status', '--porcelain'], self.fake_chromium_src.brave)

        # All l10n files should be committed, so no changes should show for them
        self.assertNotIn('M test_strings.grd', status_output)
        self.assertNotIn('M test_strings.grdp', status_output)
        self.assertNotIn('M test_strings.xtb', status_output)
        self.assertNotIn('?? untracked.grd', status_output)
        self.assertNotIn('?? untracked.grdp', status_output)
        self.assertNotIn('?? untracked.xtb', status_output)

        # Verify that the non-l10n file was not staged
        self.assertIn('?? not_l10n.txt', status_output)

        # Verify that the correct files were committed in the most recent commit
        committed_files = self.fake_chromium_src._run_git_command(
            ['show', '--name-only', '--pretty=format:'],
            self.fake_chromium_src.brave).strip().split('\n')
        committed_files = [f for f in committed_files
                           if f]  # Remove empty strings

        # Should contain all l10n files (both tracked modifications and new
        # untracked files)
        self.assertIn('test_strings.grd', committed_files)
        self.assertIn('test_strings.grdp', committed_files)
        self.assertIn('test_strings.xtb', committed_files)
        self.assertIn('untracked.grd', committed_files)
        self.assertIn('untracked.grdp', committed_files)
        self.assertIn('untracked.xtb', committed_files)

        # Should not contain non-l10n files
        self.assertNotIn('not_l10n.txt', committed_files)


if __name__ == '__main__':
    unittest.main()
