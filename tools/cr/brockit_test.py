#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

import json
import os
import subprocess
import unittest
from pathlib import Path
import shutil
from datetime import datetime
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

import brockit
from brockit import ApplyPatchesRecord

from test.fake_chromium_repo import FakeChromiumRepo


class BrockitTest(unittest.TestCase):
    """Test the patchfile generation and application."""

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumRepo()
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
            'input_file_parsers.cc').resolve()
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


class MarkChangeTaskTest(unittest.TestCase):
    """Tests for the `reassign` and `drop` change-marking commands."""

    def setUp(self):
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)
        self.brave = self.fake_chromium_src.brave

    def _git(self, *args: str) -> str:
        return self.fake_chromium_src._run_git_command(list(args), self.brave)

    def _make_target(self, subject: str) -> str:
        """Commits a change with `subject` and returns its `%h` short hash."""
        self.fake_chromium_src.write_and_stage_file('foo.txt', 'content',
                                                    self.brave)
        self.fake_chromium_src.commit(subject, self.brave)
        return self._git('log', '-1', '--format=%h')

    def _assert_marks_change(self, task, prefix: str) -> None:
        """Runs `task` over a fresh target and asserts it produced an empty
        `<prefix><hash>! <subject>` commit on top."""
        short_hash = self._make_target('A change to mark')

        task.execute(change='HEAD')

        self.assertEqual(self._git('log', '-1', '--format=%s'),
                         f'{prefix}{short_hash}! A change to mark')
        # The marking commit must be empty: no diff against its parent.
        self.assertEqual(self._git('diff', '--name-only', 'HEAD~1', 'HEAD'),
                         '')

    def test_reassign_creates_empty_reassign_commit(self):
        self._assert_marks_change(brockit.Reassign(), 'reassign!')

    def test_drop_creates_empty_drop_commit(self):
        self._assert_marks_change(brockit.Drop(), 'drop!')

    def test_drop_rejects_staged_files(self):
        """Marking refuses to run while there are staged changes."""
        self._make_target('A change to mark')
        self.fake_chromium_src.write_and_stage_file('staged.txt', 'wip',
                                                    self.brave)
        with self.assertRaises(brockit.InvalidInputException):
            brockit.Drop().execute(change='HEAD')


ISSUE_URL = 'https://github.com/brave/brave-browser/issues/4242'
PR_URL = 'https://github.com/brave/brave-core/pull/123'

# The CI labels every PR opened by `GitHubIssue` must carry. They are stored
# exactly as the command passes them to `gh` (i.e. wrapped in double quotes).
REQUIRED_CI_LABELS = [
    '"CI/run-linux-arm64"',
    '"CI/run-macos-x64"',
    '"CI/run-windows-x86"',
    '"CI/run-windows-arm64"',
]


def _values_after(cmd: list[str], flag: str) -> list[str]:
    """Returns every argument that immediately follows `flag` in `cmd`."""
    return [cmd[i + 1] for i, arg in enumerate(cmd) if arg == flag]


class _FakeGh:
    """Stand-in for `terminal.run` that answers the `gh` calls `GitHubIssue`
    makes.

    Every invocation is recorded in `self.calls`, and the canned responses are
    configured through the constructor so each test can drive a specific code
    path without touching the network or the real `gh` CLI.
    """

    def __init__(self,
                 *,
                 logged_in: bool = True,
                 issue_list: list | None = None,
                 issue_create_url: str = ISSUE_URL,
                 pr_list: list | None = None,
                 pr_create_url: str = PR_URL,
                 pr_create_error: Exception | None = None,
                 milestones: list | None = None) -> None:
        self.logged_in = logged_in
        self.issue_list = issue_list if issue_list is not None else []
        self.issue_create_url = issue_create_url
        self.pr_list = pr_list if pr_list is not None else []
        self.pr_create_url = pr_create_url
        self.pr_create_error = pr_create_error
        self.milestones = milestones if milestones is not None else []
        self.calls: list[list[str]] = []

    def __call__(self, cmd, **kwargs) -> SimpleNamespace:
        cmd = [str(x) for x in cmd]
        self.calls.append(cmd)
        verb = cmd[1:3]
        if verb == ['auth', 'status']:
            if not self.logged_in:
                raise subprocess.CalledProcessError(1, cmd, stderr='no auth')
            return SimpleNamespace(
                stdout='Logged in to github.com account fake')
        if verb == ['issue', 'list']:
            return SimpleNamespace(stdout=json.dumps(self.issue_list))
        if verb == ['issue', 'create']:
            return SimpleNamespace(stdout=f'{self.issue_create_url}\n')
        if verb == ['issue', 'edit']:
            return SimpleNamespace(stdout='')
        if verb == ['pr', 'list']:
            return SimpleNamespace(stdout=json.dumps(self.pr_list))
        if verb == ['pr', 'create']:
            if self.pr_create_error is not None:
                raise self.pr_create_error
            return SimpleNamespace(stdout=f'{self.pr_create_url}\n')
        if cmd[1] == 'api':
            if cmd[2] == '-X':  # PATCH to set the milestone.
                return SimpleNamespace(stdout='')
            return SimpleNamespace(stdout=json.dumps(self.milestones))
        raise AssertionError(f'Unexpected gh call: {cmd}')

    def call_matching(self, *prefix: str) -> list[str] | None:
        """Returns the first recorded call whose start matches `prefix`."""
        prefix = list(prefix)
        return next((c for c in self.calls if c[:len(prefix)] == prefix), None)

    def pr_create_cmd(self) -> list[str] | None:
        return self.call_matching('gh', 'pr', 'create')

    def issue_create_cmd(self) -> list[str] | None:
        return self.call_matching('gh', 'issue', 'create')


class GitHubIssueTest(unittest.TestCase):
    """Tests for the `GitHubIssue` task (issue creation and PR pushing)."""

    def setUp(self):
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        brockit.VERSION_UPGRADE_FILE = (self.fake_chromium_src.brave /
                                        '.version_upgrade')
        self.addCleanup(self.fake_chromium_src.cleanup)

    # -- helpers ---------------------------------------------------------------

    def _make_issue(self,
                    base: str = '134.0.7035.0',
                    target: str = '134.0.7037.1') -> brockit.GitHubIssue:
        return brockit.GitHubIssue(base_version=brockit.Version(base),
                                   target_version=brockit.Version(target))

    def _patch_gh(self, fake: _FakeGh) -> _FakeGh:
        patcher = patch.object(brockit.terminal, 'run', side_effect=fake)
        patcher.start()
        self.addCleanup(patcher.stop)
        return fake

    def _patch_branch(self,
                      *,
                      upstream: str | None,
                      uplift_branch: str = '1.0.x',
                      current: str = 'cr-branch') -> None:
        """Patches branch resolution so `create_push_request` runs hermetically.

        `upstream` is returned verbatim by `_get_current_branch_upstream_name`
        (use an `origin/...` value to exercise the remote-prefix stripping).
        """
        for patcher in (
                patch.object(brockit.repository.Repository,
                             'current_branch',
                             return_value=current),
                patch.object(brockit,
                             '_get_current_branch_upstream_name',
                             return_value=upstream),
                patch.object(brockit.versioning,
                             'get_uplift_branch_name_from_package',
                             return_value=uplift_branch),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    ############################################################################
    #### compose_issue_title

    def test_compose_issue_title_minor(self):
        title = self._make_issue('134.0.7035.0',
                                 '134.0.7037.1').compose_issue_title()
        self.assertEqual(
            title, 'Upgrade from Chromium 134.0.7035.0 to Chromium '
            '134.0.7037.1')

    def test_compose_issue_title_major(self):
        # Major upgrades only reference the major version numbers.
        title = self._make_issue('134.0.7035.0',
                                 '135.0.7037.1').compose_issue_title()
        self.assertEqual(title, 'Upgrade from Chromium 134 to Chromium 135')

    ############################################################################
    #### lookup_issue

    def test_lookup_issue_found(self):
        title = 'Upgrade from Chromium 134.0.7035.0 to Chromium 134.0.7037.1'
        gh = self._patch_gh(
            _FakeGh(issue_list=[{
                'number': 1,
                'title': 'Some other issue',
                'url': 'u1',
                'body': 'b1'
            }, {
                'number': 2,
                'title': title,
                'url': ISSUE_URL,
                'body': 'b2'
            }]))
        issue = self._make_issue().lookup_issue(title)
        self.assertIsNotNone(issue)
        self.assertEqual(issue['number'], 2)
        self.assertEqual(issue['url'], ISSUE_URL)
        # The search is delegated to `gh issue list`.
        self.assertIsNotNone(gh.call_matching('gh', 'issue', 'list'))

    def test_lookup_issue_not_found_when_title_differs(self):
        # `gh` does fuzzy matching, so an exact-title check is applied locally.
        self._patch_gh(
            _FakeGh(issue_list=[{
                'number': 1,
                'title': 'A close but different title',
                'url': 'u1',
                'body': 'b1'
            }]))
        self.assertIsNone(self._make_issue().lookup_issue('Exact title'))

    def test_lookup_issue_empty(self):
        self._patch_gh(_FakeGh(issue_list=[]))
        self.assertIsNone(self._make_issue().lookup_issue('Any title'))

    ############################################################################
    #### create_push_request

    def test_create_push_request_raises_on_detached_head(self):
        """A detached HEAD has no branch to open a PR from."""
        repo = self.fake_chromium_src.brave
        head = self.fake_chromium_src._run_git_command(['rev-parse', 'HEAD'],
                                                       repo)
        self.fake_chromium_src._run_git_command(['checkout', head], repo)

        with self.assertRaises(brockit.InvalidInputException):
            self._make_issue().create_push_request(ISSUE_URL)

    def test_create_push_request_raises_without_upstream(self):
        """A branch without an upstream cannot resolve a PR base."""
        repo = self.fake_chromium_src.brave
        self.fake_chromium_src._run_git_command(
            ['checkout', '-b', 'no-upstream'], repo)

        with self.assertRaises(brockit.InvalidInputException):
            self._make_issue().create_push_request(ISSUE_URL)

    def test_create_push_request_skips_when_pr_exists(self):
        """An already-open PR for the branch short-circuits creation."""
        self._patch_branch(upstream='origin/master')
        gh = self._patch_gh(_FakeGh(pr_list=[{'number': 9, 'url': PR_URL}]))

        self._make_issue().create_push_request(ISSUE_URL)

        self.assertIsNone(gh.pr_create_cmd())

    def test_create_push_request_minor_on_master(self):
        self._patch_branch(upstream='origin/master', uplift_branch='134.0.x')
        gh = self._patch_gh(_FakeGh())

        self._make_issue('134.0.7035.0',
                         '134.0.7037.1').create_push_request(ISSUE_URL)

        cmd = gh.pr_create_cmd()
        self.assertIsNotNone(cmd)
        # The base is the upstream branch with the remote prefix stripped.
        self.assertEqual(_values_after(cmd, '--base'), ['master'])
        # The PR body links back to the issue.
        self.assertIn(f'Resolves {ISSUE_URL}', cmd)
        labels = _values_after(cmd, '--label')
        for label in REQUIRED_CI_LABELS:
            self.assertIn(label, labels)
        self.assertIn('"CI/run-audit-deps"', labels)
        self.assertIn('"CI/run-network-audit"', labels)
        # `master` always runs upstream tests, but minor bumps are not drafts
        # and carry no storybook label.
        self.assertIn('"CI/run-upstream-tests"', labels)
        self.assertNotIn('"CI/storybook-url"', labels)
        self.assertNotIn('--draft', cmd)
        # 134 is even, so Emerick and Alexey are assigned.
        assignees = _values_after(cmd, '--assignee')
        self.assertEqual(assignees,
                         ['cdesouza-chromium', 'emerick', 'AlexeyBarabash'])
        # A minor, non-uplift PR title carries no branch tag.
        self.assertEqual(
            _values_after(cmd, '--title'),
            ['Upgrade from Chromium 134.0.7035.0 to Chromium 134.0.7037.1'])

    def test_create_push_request_major_is_draft_with_extra_labels(self):
        self._patch_branch(upstream='origin/master', uplift_branch='135.0.x')
        gh = self._patch_gh(_FakeGh())

        self._make_issue('134.0.7035.0',
                         '135.0.7037.1').create_push_request(ISSUE_URL)

        cmd = gh.pr_create_cmd()
        labels = _values_after(cmd, '--label')
        for label in REQUIRED_CI_LABELS:
            self.assertIn(label, labels)
        self.assertIn('"CI/run-upstream-tests"', labels)
        self.assertIn('"CI/storybook-url"', labels)
        self.assertIn('--draft', cmd)
        # 135 is odd, so Max and Sam are assigned.
        assignees = _values_after(cmd, '--assignee')
        self.assertEqual(assignees,
                         ['cdesouza-chromium', 'mkarolin', 'samartnik'])

    def test_create_push_request_uplift_sets_milestone(self):
        self._patch_branch(upstream='origin/1.70.x', uplift_branch='1.70.x')
        gh = self._patch_gh(
            _FakeGh(milestones=[{
                'number': 77,
                'title': '1.70.x - Some release'
            }, {
                'number': 1,
                'title': '1.69.x - Older release'
            }]))

        self._make_issue('134.0.7035.0',
                         '134.0.7037.1').create_push_request(ISSUE_URL)

        cmd = gh.pr_create_cmd()
        # Uplift PR titles are tagged with the target branch.
        self.assertEqual(_values_after(cmd, '--title'), [
            '[1.70.x] Upgrade from Chromium 134.0.7035.0 to Chromium '
            '134.0.7037.1'
        ])
        # Uplifts of minor bumps don't run upstream tests.
        self.assertNotIn('"CI/run-upstream-tests"',
                         _values_after(cmd, '--label'))
        # Required CI labels still apply to uplifts.
        for label in REQUIRED_CI_LABELS:
            self.assertIn(label, _values_after(cmd, '--label'))
        # The milestone matching the upstream branch is PATCHed onto the PR.
        patch_cmd = gh.call_matching('gh', 'api', '-X', 'PATCH')
        self.assertIsNotNone(patch_cmd)
        self.assertIn('repos/brave/brave-core/issues/123', patch_cmd)
        self.assertIn('milestone=77', patch_cmd)

    def test_create_push_request_uplift_no_milestones_raises(self):
        self._patch_branch(upstream='origin/1.70.x', uplift_branch='1.70.x')
        self._patch_gh(_FakeGh(milestones=[]))

        with self.assertRaises(brockit.BadOutcomeException):
            self._make_issue().create_push_request(ISSUE_URL)

    def test_create_push_request_uplift_milestone_not_found_raises(self):
        self._patch_branch(upstream='origin/1.70.x', uplift_branch='1.70.x')
        self._patch_gh(
            _FakeGh(milestones=[{
                'number': 1,
                'title': '1.69.x - Older release'
            }]))

        with self.assertRaises(brockit.BadOutcomeException):
            self._make_issue().create_push_request(ISSUE_URL)

    def test_create_push_request_pr_creation_failure_raises(self):
        self._patch_branch(upstream='origin/master', uplift_branch='134.0.x')
        self._patch_gh(
            _FakeGh(pr_create_error=subprocess.CalledProcessError(
                1, ['gh', 'pr', 'create'], stderr='gh blew up')))

        with self.assertRaises(brockit.BadOutcomeException):
            self._make_issue().create_push_request(ISSUE_URL)

    ############################################################################
    #### create_or_update_version_issue

    def test_create_or_update_creates_new_issue(self):
        gh = self._patch_gh(_FakeGh(issue_list=[]))

        with patch.object(brockit.GitHubIssue,
                          'create_push_request') as mock_push:
            self._make_issue().create_or_update_version_issue(with_pr=False)

        cmd = gh.issue_create_cmd()
        self.assertIsNotNone(cmd)
        labels = _values_after(cmd, '--label')
        self.assertIn('"Chromium/upgrade minor"', labels)
        self.assertIn('"QA/Yes"', labels)
        self.assertEqual(
            _values_after(cmd, '--title'),
            ['Upgrade from Chromium 134.0.7035.0 to Chromium 134.0.7037.1'])
        # No PR is pushed when with_pr is False.
        mock_push.assert_not_called()

    def test_create_or_update_creates_issue_and_pushes_pr(self):
        self._patch_gh(_FakeGh(issue_list=[]))

        with patch.object(brockit.GitHubIssue,
                          'create_push_request') as mock_push:
            self._make_issue().create_or_update_version_issue(with_pr=True)

        # The freshly created issue URL is passed to the push request.
        mock_push.assert_called_once_with(ISSUE_URL)

    def test_create_or_update_existing_issue_up_to_date(self):
        issue = self._make_issue('134.0.7035.0', '134.0.7037.1')
        title = issue.compose_issue_title()
        link = issue.target_version.get_googlesource_diff_link(
            from_version=str(issue.base_version))
        # The existing body already points at the current diff link.
        gh = self._patch_gh(
            _FakeGh(issue_list=[{
                'number': 5,
                'title': title,
                'url': ISSUE_URL,
                'body': f'Some text\n{link}\nmore text'
            }]))

        with patch.object(brockit.GitHubIssue, 'create_push_request'):
            issue.create_or_update_version_issue(with_pr=False)

        # An up-to-date issue is neither edited nor recreated.
        self.assertIsNone(gh.call_matching('gh', 'issue', 'edit'))
        self.assertIsNone(gh.issue_create_cmd())

    def test_create_or_update_existing_issue_updates_body(self):
        issue = self._make_issue('134.0.7035.0', '134.0.7037.1')
        title = issue.compose_issue_title()
        stale_link = ('https://chromium.googlesource.com/chromium/src/+log/'
                      '111.0.0.0..112.0.0.0')
        gh = self._patch_gh(
            _FakeGh(issue_list=[{
                'number': 5,
                'title': title,
                'url': ISSUE_URL,
                'body': f'Some text\n{stale_link}\nmore text'
            }]))

        with patch.object(brockit.GitHubIssue, 'create_push_request'):
            issue.create_or_update_version_issue(with_pr=False)

        # A stale diff link triggers an edit, not a brand new issue.
        edit_cmd = gh.call_matching('gh', 'issue', 'edit')
        self.assertIsNotNone(edit_cmd)
        self.assertIn('5', edit_cmd)
        self.assertIsNone(gh.issue_create_cmd())

    ############################################################################
    #### execute

    def test_execute_raises_when_not_logged_in(self):
        self._patch_gh(_FakeGh(logged_in=False))

        with self.assertRaises(brockit.BadOutcomeException):
            self._make_issue().execute()

    def test_execute_creates_issue_with_pr_when_logged_in(self):
        self._patch_gh(_FakeGh(logged_in=True))

        with patch.object(brockit.GitHubIssue,
                          'create_or_update_version_issue') as mock_create:
            self._make_issue().execute()

        mock_create.assert_called_once_with(with_pr=True)


class MergeTest(unittest.TestCase):
    """Tests for the `merge` command."""

    def setUp(self):
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)
        self.brave = self.fake_chromium_src.brave
        self.remote = self.fake_chromium_src.remote / 'brave'

    def _git(self, *args: str, repo: Path | None = None) -> str:
        return self.fake_chromium_src._run_git_command(list(args), repo
                                                       or self.brave)

    def _setup_upstream(self) -> None:
        """Wires up an `origin/master` upstream for a fresh `cr149` branch.
        """
        self.fake_chromium_src.create_brave_remote()
        self._git('config',
                  'receive.denyCurrentBranch',
                  'ignore',
                  repo=self.remote)
        # The freshly-initialised remote carries its own unrelated "Initial
        # commit" on `master`; force-push so `origin/master` starts from this
        # repo's history and later fast-forwards line up.
        self._git('push', '--force', 'origin', 'HEAD:master')
        self._git('checkout', '-b', 'cr149')
        self._git('branch', '--set-upstream-to=origin/master')

    def _advance_upstream(self, relative_path: str, content: str) -> str:
        """Adds a commit to `origin/master` that `cr149` does not have.

        Commits `content` to `relative_path` on a throwaway branch based on
        `origin/master`, pushes it, and returns to `cr149`. Returns the new
        remote `master` hash.
        """
        self._git('checkout', '-b', '_tmp_upstream', 'origin/master')
        self.fake_chromium_src.write_and_stage_file(relative_path, content,
                                                    self.brave)
        self.fake_chromium_src.commit(f'Upstream change to {relative_path}',
                                      self.brave)
        self._git('push', 'origin', 'HEAD:master')
        self._git('checkout', 'cr149')
        self._git('branch', '-D', '_tmp_upstream')
        return self._git('rev-parse', 'refs/heads/master', repo=self.remote)

    def _remote_master(self) -> str:
        return self._git('rev-parse', 'refs/heads/master', repo=self.remote)

    def test_merge_raises_when_detached_head(self):
        self._setup_upstream()
        self._git('checkout', '--detach', 'HEAD')
        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

    def test_merge_raises_without_upstream(self):
        self._git('checkout', '-b', 'no-upstream')
        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

    def test_merge_raises_when_nothing_to_merge(self):
        """A branch already at its upstream has nothing to push."""
        self._setup_upstream()
        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

    def test_merge_raises_on_dirty_tree(self):
        self._setup_upstream()
        self.fake_chromium_src.write_and_stage_file('foo.txt', 'wip',
                                                    self.brave)
        self.fake_chromium_src.commit('A branch change', self.brave)
        # Leave an uncommitted modification behind.
        (self.brave / 'foo.txt').write_text('dirty')
        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

    def test_merge_rejects_when_merge_in_progress(self):
        """A leftover `MERGE_HEAD` is rejected rather than being concluded."""
        self._setup_upstream()
        self.fake_chromium_src.write_and_stage_file('conflict.txt', 'branch',
                                                    self.brave)
        self.fake_chromium_src.commit('Branch side', self.brave)
        self._advance_upstream('conflict.txt', 'upstream')
        # Kick off a conflicting merge and leave it unresolved.
        with self.assertRaises(subprocess.CalledProcessError):
            self._git('merge', 'origin/master')
        self.assertTrue(
            brockit.repository.brave.is_valid_git_reference('MERGE_HEAD'))

        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

    def test_merge_rejects_when_rebase_in_progress(self):
        """A half-finished rebase blocks the merge until it is concluded."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty('[cr149] Feature A', self.brave)
        self.fake_chromium_src.commit_empty('[cr149] Feature B', self.brave)
        # Start an interactive rebase that stops on an `edit` command.
        env = {**os.environ, 'GIT_SEQUENCE_EDITOR': "sed -i '1s/^pick/edit/'"}
        subprocess.run(['git', 'rebase', '-i', 'HEAD~2'],
                       cwd=self.brave,
                       env=env,
                       capture_output=True,
                       text=True,
                       check=True)
        self.assertTrue(brockit.repository.brave.is_rebase_in_progress())

        try:
            with self.assertRaises(brockit.InvalidInputException):
                brockit.Merge().execute()
        finally:
            subprocess.run(['git', 'rebase', '--abort'],
                           cwd=self.brave,
                           capture_output=True,
                           check=False)

    def test_merge_fast_forward_pushes_branch(self):
        """A branch strictly ahead of its upstream pushes without a merge
        commit."""
        self._setup_upstream()
        self.fake_chromium_src.write_and_stage_file('foo.txt', 'one',
                                                    self.brave)
        self.fake_chromium_src.commit('First branch change', self.brave)
        self.fake_chromium_src.write_and_stage_file('bar.txt', 'two',
                                                    self.brave)
        self.fake_chromium_src.commit('Second branch change', self.brave)
        head = self._git('rev-parse', 'HEAD')

        brockit.Merge().execute()

        # The remote master now points at the branch tip, and no merge commit
        # was created (HEAD is unchanged).
        self.assertEqual(self._remote_master(), head)
        self.assertEqual(self._git('rev-parse', 'HEAD'), head)

    def test_merge_diverged_creates_merge_commit_and_pushes(self):
        """When histories diverge, a merge commit is created and pushed."""
        self._setup_upstream()
        self.fake_chromium_src.write_and_stage_file('branch.txt', 'branch',
                                                    self.brave)
        self.fake_chromium_src.commit('Branch change', self.brave)
        self._advance_upstream('upstream.txt', 'upstream')

        brockit.Merge().execute()

        head = self._git('rev-parse', 'HEAD')
        # The new HEAD is a merge commit (two parents) and the remote master
        # was fast-forwarded to it.
        self.assertEqual(len(self._git('rev-parse', 'HEAD^@').split()), 2)
        self.assertEqual(self._remote_master(), head)

    def test_merge_conflict_rolls_back_and_fails(self):
        """Merge conflicts roll the merge back and fail without pushing."""
        self._setup_upstream()
        self.fake_chromium_src.write_and_stage_file('conflict.txt', 'branch',
                                                    self.brave)
        self.fake_chromium_src.commit('Branch side', self.brave)
        head_before = self._git('rev-parse', 'HEAD')
        upstream_master = self._advance_upstream('conflict.txt', 'upstream')

        with self.assertRaises(brockit.BadOutcomeException):
            brockit.Merge().execute()

        # The merge was aborted: no leftover merge state, the branch tip is
        # unchanged, and nothing was pushed.
        self.assertFalse(
            brockit.repository.brave.is_valid_git_reference('MERGE_HEAD'))
        self.assertEqual(self._git('rev-parse', 'HEAD'), head_before)
        self.assertEqual(self._remote_master(), upstream_master)

    def test_merge_blocks_on_unsquashed_minor_bumps(self):
        """Two un-squashed minor bumps mean the branch still needs a rebase."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty(
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1.', self.brave)
        self.fake_chromium_src.commit_empty(
            'Update from Chromium 1.0.0.1 to Chromium 1.0.0.2.', self.brave)
        master_before = self._remote_master()

        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

        # The branch was never pushed.
        self.assertEqual(self._remote_master(), master_before)

    def test_merge_blocks_on_pending_fixup(self):
        """A pending `fixup!` commit blocks the merge until it is squashed."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty('[cr149] Feature A', self.brave)
        self.fake_chromium_src.commit_empty('fixup! [cr149] Feature A',
                                            self.brave)
        master_before = self._remote_master()

        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

        self.assertEqual(self._remote_master(), master_before)

    def test_merge_blocks_on_orphaned_fixup(self):
        """An orphaned `fixup!` (no matching target) blocks the merge.

        Git's autosquash leaves it as a `pick`, so only the orphan filter
        catches it -- unlike an attached fixup, whose `fixup` command already
        differs from the identity plan.
        """
        self._setup_upstream()
        self.fake_chromium_src.commit_empty('[cr149] Feature A', self.brave)
        self.fake_chromium_src.commit_empty(
            'fixup! [cr149] A commit that is not on this branch', self.brave)
        master_before = self._remote_master()

        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

        self.assertEqual(self._remote_master(), master_before)

    def _assert_tag_blocks_merge(self, subject: str) -> None:
        """Asserts a commit with `subject` blocks the merge without pushing."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty('[cr149] Feature A', self.brave)
        self.fake_chromium_src.commit_empty(subject, self.brave)
        master_before = self._remote_master()

        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute()

        self.assertEqual(self._remote_master(), master_before)

    def test_merge_blocks_on_wip_commit_lowercase(self):
        """A `[wip]` commit blocks the merge and is never pushed."""
        self._assert_tag_blocks_merge('[wip] Not done yet')

    def test_merge_blocks_on_wip_commit_uppercase(self):
        """A `[WIP]` commit blocks the merge and is never pushed."""
        self._assert_tag_blocks_merge('[WIP] Not done yet')

    def test_merge_blocks_on_do_not_merge_tag(self):
        """A `[DO NOT MERGE]` commit blocks the merge and is never pushed."""
        self._assert_tag_blocks_merge('[DO NOT MERGE] keep this local')

    def test_merge_blocks_on_do_not_tag_case_insensitive(self):
        """`[DO NOT ...]` matching is case-insensitive and tag-agnostic."""
        self._assert_tag_blocks_merge('[Do Not Land] experimental change')

    def test_never_merge_tag_regex_only_matches_bracketed_tags(self):
        """The tag regex matches only bracketed `[wip]` / `[DO NOT ...]` tags,
        never a plain "WIP" / "do not" word elsewhere in the subject."""
        regex = brockit.Merge._NEVER_MERGE_TAG_RE
        matches = [
            '[wip]',
            '[WIP]',
            '[Wip]',
            '[DO NOT MERGE]',
            '[do not land]',
            '[Do Not Submit] foo',
            'pick abc # [wip] subject # empty',
        ]
        for subject in matches:
            with self.subTest(subject=subject):
                self.assertIsNotNone(regex.search(subject))

        non_matches = [
            'Rework the WIP handling logic',
            'wip cleanup without brackets',
            'do not remove this comment',
            'DO NOT panic',
            '[cr149] WIP feature',
            '[cr149] refactor: do not inline',
        ]
        for subject in non_matches:
            with self.subTest(subject=subject):
                self.assertIsNone(regex.search(subject))

    def test_merge_allows_wip_word_outside_tag(self):
        """A plain "WIP" word (no `[wip]` tag) does not block the merge."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty('[cr149] Rework WIP handling',
                                            self.brave)
        head = self._git('rev-parse', 'HEAD')

        brockit.Merge().execute()

        self.assertEqual(self._remote_master(), head)

    def test_merge_allows_canonical_branch(self):
        """A single, correctly-placed pinned commit is merge-ready."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty(
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1.', self.brave)
        self.fake_chromium_src.commit_empty('[cr149] Feature A', self.brave)
        head = self._git('rev-parse', 'HEAD')

        brockit.Merge().execute()

        self.assertEqual(self._remote_master(), head)

    def test_merge_dry_run_does_not_push(self):
        """`--dry-run` validates a ready branch but pushes nothing."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty('[cr149] Feature A', self.brave)
        head_before = self._git('rev-parse', 'HEAD')
        master_before = self._remote_master()

        brockit.Merge().execute(dry_run=True)

        # HEAD is unchanged (no merge commit) and the remote was not pushed.
        self.assertEqual(self._git('rev-parse', 'HEAD'), head_before)
        self.assertEqual(self._remote_master(), master_before)

    def test_merge_dry_run_still_reports_unready_branch(self):
        """`--dry-run` runs the readiness pre-check and still errors."""
        self._setup_upstream()
        self.fake_chromium_src.commit_empty('[cr149] Feature A', self.brave)
        self.fake_chromium_src.commit_empty('fixup! [cr149] Feature A',
                                            self.brave)

        with self.assertRaises(brockit.InvalidInputException):
            brockit.Merge().execute(dry_run=True)


if __name__ == '__main__':
    unittest.main()
