#!/usr/bin/env vpython3
# # Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
from pathlib import Path
import unittest

from test.fake_chromium_repo import FakeChromiumRepo


class FakeChromiumRepoTest(unittest.TestCase):

    def setUp(self):
        """Set up a FakeChromiumRepo instance for each test."""
        self.repo = FakeChromiumRepo()

    def tearDown(self):
        """Clean up the FakeChromiumRepo instance after each test."""
        self.repo.cleanup()

    def test_add_repo(self):
        """Tests the add_repo method of FakeChromiumRepo."""
        new_repo_path = 'new_repo'
        self.repo.add_repo(new_repo_path)

        # Verify the new repository exists
        new_repo_full_path = self.repo.chromium / new_repo_path
        self.assertTrue(new_repo_full_path.exists())
        self.assertTrue((new_repo_full_path / '.git').exists())

        # Verify the initial commit exists
        log = self.repo._run_git_command(['log', '--oneline'],
                                         new_repo_full_path)
        self.assertIn('Initial commit', log)

    def test_add_dep(self):
        """Tests the add_dep method of FakeChromiumRepo."""
        dep_path = 'third_party/dep_repo'
        self.repo.add_dep(dep_path)

        # Verify the dependency repository exists
        dep_full_path = self.repo.chromium / dep_path
        self.assertTrue(dep_full_path.exists())
        self.assertTrue((dep_full_path / '.git').exists())

        # Verify the submodule is added to the main repository
        submodule_config = self.repo._run_git_command([
            'config', '--file', '.gitmodules', '--get',
            f'submodule.{dep_path}.path'
        ], self.repo.chromium)
        self.assertEqual(submodule_config, dep_path)

        # Verify the submodule commit exists in the main repository
        log = self.repo._run_git_command(['log', '--oneline'],
                                         self.repo.chromium)
        self.assertIn(f'Add submodule {dep_path}', log)

    def test_add_tag(self):
        """Tests the add_tag method of FakeChromiumRepo."""
        version = '1.2.3.4'
        self.repo.add_tag(version)

        # Verify the VERSION file exists with correct content
        version_file = self.repo.chromium / 'chrome' / 'VERSION'
        self.assertTrue(version_file.exists())
        with version_file.open('r') as f:
            content = f.read()
        self.assertIn('MAJOR=1', content)
        self.assertIn('MINOR=2', content)
        self.assertIn('BUILD=3', content)
        self.assertIn('PATCH=4', content)

        # Verify the tag exists in the repository
        tags = self.repo._run_git_command(['tag'], self.repo.chromium)
        self.assertIn(version, tags)

        # Verify the commit message for the tag
        log = self.repo._run_git_command(['log', '-1', '--oneline'],
                                         self.repo.chromium)
        self.assertIn(f'VERSION {version}', log)

    def test_commit(self):
        """Tests the commit method of FakeChromiumRepo."""
        file_path = 'test_file.txt'
        content = 'This is a test file.'
        self.repo.write_and_stage_file(file_path, content, self.repo.chromium)

        # Commit the staged file
        commit_message = 'Add test_file.txt'
        commit_hash = self.repo.commit(commit_message, self.repo.chromium)

        # Verify the commit exists in the repository
        short_hash = commit_hash[:7]
        log = self.repo._run_git_command(['log', '--oneline'],
                                         self.repo.chromium)
        self.assertIn(commit_message, log)
        self.assertIn(short_hash, log)

    def test_commit_empty(self):
        """Tests the commit_empty method of FakeChromiumRepo."""
        commit_message = 'Empty commit for testing'
        commit_hash = self.repo.commit_empty(commit_message,
                                             self.repo.chromium)

        # Verify the empty commit exists in the repository
        short_hash = commit_hash[:7]
        log = self.repo._run_git_command(['log', '--oneline'],
                                         self.repo.chromium)
        self.assertIn(commit_message, log)
        self.assertIn(short_hash, log)

    def test_write_and_stage_file(self):
        """Tests the write_and_stage_file method of FakeChromiumRepo."""
        file_path = 'test_dir/test_file.txt'
        content = 'This is a test file.'
        self.repo.write_and_stage_file(file_path, content, self.repo.chromium)

        # Verify the file exists with the correct content
        full_file_path = self.repo.chromium / file_path
        self.assertTrue(full_file_path.exists())
        with full_file_path.open('r') as f:
            self.assertEqual(f.read(), content)

        # Verify the file is staged
        staged_files = self.repo._run_git_command(
            ['diff', '--name-only', '--cached'], self.repo.chromium)
        self.assertIn(file_path, staged_files)

    def test_update_brave_version(self):
        """Tests the update_brave_version method of FakeChromiumRepo."""
        new_version = '1.2.3.4'
        commit_hash = self.repo.update_brave_version(new_version)

        # Verify the package.json file exists
        package_json_path = self.repo.brave / 'package.json'
        self.assertTrue(package_json_path.exists())

        # Verify the version in package.json is updated
        with package_json_path.open('r') as f:
            package_data = json.load(f)
        self.assertEqual(package_data['config']['projects']['chrome']['tag'],
                         new_version)

        # Verify the commit exists in the repository
        short_hash = commit_hash[:7]
        log = self.repo._run_git_command(['log', '--oneline'], self.repo.brave)
        self.assertIn(f'Update from Chromium N/A to Chromium {new_version}',
                      log)
        self.assertIn(short_hash, log)

    def test_update_patches(self):
        """Tests the update_patches method of FakeChromiumRepo."""
        # Add a file to the Chromium repo and commit it
        file_path_chromium = 'modified_file_chromium.txt'
        content_chromium = 'This is a modified file in the Chromium repo.'
        self.repo.write_and_stage_file(file_path_chromium, content_chromium,
                                       self.repo.chromium)
        self.repo.commit('Add file to Chromium repo', self.repo.chromium)

        # Add a file to another repo and commit it
        repo_path = 'test_repo'
        self.repo.add_repo(repo_path)
        file_path_repo = 'modified_file_repo.txt'
        content_repo = 'This is a modified file in the test_repo.'
        self.repo.write_and_stage_file(file_path_repo, content_repo,
                                       self.repo.chromium / repo_path)
        self.repo.commit('Add file to test_repo',
                         self.repo.chromium / repo_path)

        # Run update_patches on a clean tree (no patches should be produced)
        self.repo.run_update_patches()
        self.assertFalse(any(self.repo.brave_patches.iterdir()),
                         'No patches should be produced for a clean tree.')

        # Modify the files without staging (leave the tree dirty)
        new_content_chromium = 'Modified content in Chromium repo.'
        full_file_path_chromium = self.repo.chromium / file_path_chromium
        full_file_path_chromium.write_text(new_content_chromium)

        new_content_repo = 'Modified content in test_repo.'
        full_file_path_repo = self.repo.chromium / repo_path / file_path_repo
        full_file_path_repo.write_text(new_content_repo)

        # Run update_patches again (patches should now be produced)
        self.repo.run_update_patches()

        # Check that the patchfile for test_repo is created
        patch_file_name_repo = f'{file_path_repo.replace("/", "-")}.patch'
        patch_file_path_repo = (self.repo.brave_patches / repo_path /
                                patch_file_name_repo)
        self.assertTrue(patch_file_path_repo.exists())

        # Check the patchfile for Chromium is created
        patch_file_name_chromium = (
            f'{file_path_chromium.replace("/", "-")}.patch')
        patch_file_path_chromium = (self.repo.brave_patches /
                                    patch_file_name_chromium)
        self.assertTrue(patch_file_path_chromium.exists())

        # Verify the patch file for the test_repo contains the correct diff
        with patch_file_path_repo.open('r') as f:
            patch_content_repo = f.read()
        self.assertIn('diff --git a/', patch_content_repo)
        self.assertIn('b/', patch_content_repo)
        self.assertIn(new_content_repo, patch_content_repo)

        # Verify the patch file for Chromium contains the correct diff
        with patch_file_path_chromium.open('r') as f:
            patch_content_chromium = f.read()
        self.assertIn('diff --git a/', patch_content_chromium)
        self.assertIn('b/', patch_content_chromium)
        self.assertIn(new_content_chromium, patch_content_chromium)

    def test_run_apply_patches(self):
        """Tests the run_apply_patches method of FakeChromiumRepo."""
        # Add a file to the Chromium repo and commit it
        file_path_chromium = 'modified_file_chromium.txt'
        content_chromium = ('This is the original chromium content\n'
                            'Second line waiting change\n'
                            'Final line.')
        self.repo.write_and_stage_file(file_path_chromium, content_chromium,
                                       self.repo.chromium)
        self.repo.commit('Add file to Chromium repo', self.repo.chromium)

        # Add a file to another repo and commit it
        repo_path = 'test_repo'
        self.repo.add_repo(repo_path)
        file_path_repo = 'modified_file_repo.txt'
        content_repo = ('This is the original test_repo content\n'
                        'Second line waiting change\n'
                        'Final line.')
        self.repo.write_and_stage_file(file_path_repo, content_repo,
                                       self.repo.chromium / repo_path)
        self.repo.commit('Add file to test_repo',
                         self.repo.chromium / repo_path)

        # Modify the files without staging (leave the tree dirty)
        full_file_path_chromium = self.repo.chromium / file_path_chromium
        self.assertEqual(full_file_path_chromium.read_text(), content_chromium)
        new_content_chromium = content_chromium.replace(
            'Second line waiting change', 'Modified second line')
        full_file_path_chromium.write_text(new_content_chromium)

        new_content_repo = content_repo.replace('Second line waiting change',
                                                'Modified second line')
        full_file_path_repo = self.repo.chromium / repo_path / file_path_repo
        full_file_path_repo.write_text(new_content_repo)

        # Run update_patches to generate patches
        self.repo.run_update_patches()

        # Reset the changes to simulate a clean state
        self.repo._run_git_command(['checkout', '.'], self.repo.chromium)
        self.repo._run_git_command(['checkout', '.'],
                                   self.repo.chromium / repo_path)

        # Run apply_patches to apply the patches
        self.assertEqual(full_file_path_chromium.read_text(), content_chromium)
        self.assertEqual(full_file_path_repo.read_text(), content_repo)
        self.assertEqual([], self.repo.run_apply_patches())

        # Verify the changes from the patches are applied
        self.assertEqual(full_file_path_chromium.read_text(),
                         new_content_chromium)
        self.assertEqual(full_file_path_repo.read_text(), new_content_repo)

    def test_run_apply_patches_with_failures(self):
        """Tests run_apply_patches with conflict/corrupted patch failures."""
        # Add a file to the Chromium repo and commit it

        self.repo.write_and_stage_file('a.txt', 'contents of a.txt',
                                       self.repo.chromium)
        self.repo.write_and_stage_file('b.txt', 'contents of b.txt',
                                       self.repo.chromium)
        self.repo.write_and_stage_file('c.txt', 'contents of c.txt',
                                       self.repo.chromium)
        self.repo.commit('Adding files to chrome repo', self.repo.chromium)

        self.repo.chromium.joinpath('a.txt').write_text(
            'patched contents of a.txt')
        self.repo.chromium.joinpath('b.txt').write_text(
            'also patched contents of b.txt')

        # Run update_patches to generate patches
        self.repo.run_update_patches()

        # Reset the changes to simulate a clean state
        self.repo._run_git_command(['checkout', '.'], self.repo.chromium)

        # Run apply_patches to apply the patches
        self.assertEqual([], self.repo.run_apply_patches())

        # Reset the changes to simulate a clean state
        self.repo._run_git_command(['checkout', '.'], self.repo.chromium)

        # let's update these files to cause conflicts
        self.repo.write_and_stage_file('a.txt', 'second contents of a.txt',
                                       self.repo.chromium)
        self.repo.write_and_stage_file('b.txt', 'second contents of b.txt',
                                       self.repo.chromium)
        self.repo.commit('Updating files to chrome repo', self.repo.chromium)

        # Run apply_patches to apply the patches
        apply_failures = self.repo.run_apply_patches()
        self.assertEqual(len(apply_failures), 2)

        # Check the values in apply_failures
        failure = (next(
            (f for f in apply_failures
             if Path(f['patchPath']).as_posix() == 'patches/b.txt.patch'),
            None))
        self.assertIsNotNone(failure)
        self.assertEqual(failure['path'], 'b.txt')
        self.assertEqual(failure['reason'], 'PATCH_CHANGED')

        failure = (next(
            (f for f in apply_failures
             if Path(f['patchPath']).as_posix() == 'patches/a.txt.patch'),
            None))
        self.assertIsNotNone(failure)
        self.assertEqual(failure['path'], 'a.txt')
        self.assertEqual(failure['reason'], 'PATCH_CHANGED')

        # Reset the changes to simulate a clean state
        self.repo._run_git_command(['checkout', '.'], self.repo.chromium)

        # Commit the deletion of b.txt
        self.repo.delete_file('b.txt', self.repo.chromium)
        self.repo.commit('Delete b.txt', self.repo.chromium)

        # Run apply_patches to apply the patches
        apply_failures = self.repo.run_apply_patches()
        self.assertEqual(len(apply_failures), 2)

        # Check the values in apply_failures
        failure = (next(
            (f for f in apply_failures
             if Path(f['patchPath']).as_posix() == 'patches/b.txt.patch'),
            None))
        self.assertIsNotNone(failure)
        self.assertEqual(failure['path'], 'b.txt')
        self.assertEqual(failure['reason'], 'SRC_REMOVED')

        failure = (next(
            (f for f in apply_failures
             if Path(f['patchPath']).as_posix() == 'patches/a.txt.patch'),
            None))
        self.assertIsNotNone(failure)
        self.assertEqual(failure['path'], 'a.txt')
        self.assertEqual(failure['reason'], 'PATCH_CHANGED')

        # Reset the changes to simulate a clean state
        self.repo._run_git_command(['checkout', '.'], self.repo.chromium)

        # Corrupt the patch file for a.txt
        patch_file_a = self.repo.brave_patches / 'a.txt.patch'
        patch_file_a.write_text('corrupted patch content')

        # Run apply_patches to apply the patches
        apply_failures = self.repo.run_apply_patches()
        self.assertEqual(len(apply_failures), 2)

        # Check the values in apply_failures
        failure = (next(
            (f for f in apply_failures
             if Path(f['patchPath']).as_posix() == 'patches/b.txt.patch'),
            None))
        self.assertIsNotNone(failure)
        self.assertEqual(failure['path'], 'b.txt')
        self.assertEqual(failure['reason'], 'SRC_REMOVED')

        failure = (next(
            (f for f in apply_failures
             if Path(f['patchPath']).as_posix() == 'patches/a.txt.patch'),
            None))
        self.assertIsNotNone(failure)
        self.assertEqual(failure['path'], None)
        self.assertEqual(failure['reason'], 'PATCH_CHANGED')

    def test_delete_file(self):
        """Tests the delete_file method of FakeChromiumRepo."""
        file_path = 'test_dir/test_file.txt'
        content = 'This is a test file.'

        # Write and stage the file
        self.repo.write_and_stage_file(file_path, content, self.repo.chromium)
        full_file_path = self.repo.chromium / file_path
        self.assertTrue(full_file_path.exists())

        # Commit the file
        self.repo.commit('Add test_file.txt', self.repo.chromium)

        # Delete the file
        self.repo.delete_file(file_path, self.repo.chromium)

        # Verify the file is deleted
        self.assertFalse(full_file_path.exists())

        # Verify the deletion is staged
        staged_files = self.repo._run_git_command(
            ['diff', '--name-only', '--cached'], self.repo.chromium)
        self.assertIn(file_path, staged_files)

        # Commit the deletion
        self.repo.commit('Delete test_file.txt', self.repo.chromium)

        # Verify the deletion is committed
        self.assertIn(
            'Delete test_file.txt',
            self.repo._run_git_command(['log', '--oneline'],
                                       self.repo.chromium))


if __name__ == '__main__':
    unittest.main()
