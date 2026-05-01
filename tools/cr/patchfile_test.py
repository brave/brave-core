#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path
from patchfile import Patchfile
import repository
from repository import Repository

from test.fake_chromium_src import FakeChromiumSrc


class PatchfileTest(unittest.TestCase):
    """Test the patchfile generation and application."""

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.fake_chromium_src.add_dep('v8')
        self.fake_chromium_src.add_dep('third_party/test1')
        self.addCleanup(self.fake_chromium_src.cleanup)

    def test_get_repository_from_patch_name(self):
        """Test the result of get_repository_from_patch_name"""
        patchfile = Patchfile(
            path=Path("patches/build-android-gyp-dex.py.patch"),
            source=Path("build/android/gyp/dex.py"))
        self.assertEqual(patchfile.get_repository_from_patch_name(),
                         repository.chromium)

        patchfile = Patchfile(
            path=Path("patches/v8/build-android-gyp-dex.py.patch"),
            source=Path("build/android/gyp/dex.py"))
        self.assertEqual(patchfile.get_repository_from_patch_name(),
                         Repository(self.fake_chromium_src.chromium / 'v8'))

        patchfile = Patchfile(path=Path(
            "patches/third_party/test1/build-android-gyp-dex.py.patch"),
                              source=Path("build/android/gyp/dex.py"))
        self.assertEqual(
            patchfile.get_repository_from_patch_name(),
            Repository(self.fake_chromium_src.chromium / 'third_party/test1'))

    def test_source_name_from_patch_naming(self):
        """Test patched file name name heuristics."""
        patchfile = Patchfile(
            path=Path("patches/build-android-gyp-dex.py.patch"),
            source=Path("build/android/gyp/dex.py"))
        self.assertEqual(patchfile.source_name_from_patch_naming(),
                         "build/android/gyp/dex.py")

        patchfile = Patchfile(
            path=Path("patches/v8/build-android-gyp-dex.py.patch"),
            source=Path("build/android/gyp/dex.py"))
        self.assertEqual(patchfile.source_name_from_patch_naming(),
                         "build/android/gyp/dex.py")

        patchfile = Patchfile(path=Path(
            "patches/third_party/test1/build-android-gyp-dex.py.patch"),
                              source=Path("build/android/gyp/dex.py"))
        self.assertEqual(patchfile.source_name_from_patch_naming(),
                         "build/android/gyp/dex.py")

    def test_apply_conflict(self):
        test_idl = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_idl, """
                enum ExtensionType {
                    HOSTED_APP,
                    PLATFORM_APP,
                    LEGACY_PACKAGED_APP,
                    EXTENSION,
                    THEME,
                    SHARED_MODULE
                  };

                  enum Location {
                    FROM_STORE,
                    UNPACKED,
                    THIRD_PARTY,
                    INSTALLED_BY_DEFAULT,
                    UNKNOWN
                  };
                """, self.fake_chromium_src.chromium)

        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        # Let's create a patch for it
        target_file = self.fake_chromium_src.chromium / test_idl
        target_file.write_text(target_file.read_text().replace(
            'FROM_STORE', 'FROM_STORE,\n    FROM_BRAVE_STORE'))
        self.fake_chromium_src.run_update_patches()
        # clearing out our custom change so we can have upstream changes
        # piling to this file
        self.fake_chromium_src._run_git_command(
            ["checkout", "."], self.fake_chromium_src.chromium)
        self.assertNotIn('FROM_BRAVE_STORE', target_file.read_text())

        # Adding an upstream chromium change that should conflict with our
        # patch
        self.fake_chromium_src.write_and_stage_file(
            test_idl,
            target_file.read_text().replace('FROM_STORE',
                                            'FROM_STORE,\n    DELETED'),
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(
            'Added DELETED to developer_private.idl Location',
            self.fake_chromium_src.chromium)

        self.assertNotIn('FROM_BRAVE_STORE', target_file.read_text())
        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.CONFLICT)

    def test_apply_conflict_with_whitespace_error(self):
        """Tests the behavior when applying a patch with whitespace errors."""

        test_idl = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_idl, """
                enum ExtensionType {
                    HOSTED_APP,
                    PLATFORM_APP,
                    LEGACY_PACKAGED_APP,
                    EXTENSION,
                    THEME,
                    SHARED_MODULE
                  };

                  enum Location {
                    FROM_STORE,
                    UNPACKED,
                    THIRD_PARTY,
                    INSTALLED_BY_DEFAULT,
                    UNKNOWN
                  };
                """, self.fake_chromium_src.chromium)

        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        # Let's create a patch for it
        target_file = self.fake_chromium_src.chromium / test_idl
        target_file.write_text(target_file.read_text().replace(
            'FROM_STORE', 'FROM_STORE,\n    FROM_BRAVE_STORE'))

        # Let's create a patch with trailing spaces
        target_file.write_text(target_file.read_text().replace(
            'UNKNOWN', 'UNKNOWN,\n    ANOTHER '))

        self.fake_chromium_src.run_update_patches()
        # clearing out our custom change so we can have upstream changes
        # piling to this file
        self.fake_chromium_src._run_git_command(
            ["checkout", "."], self.fake_chromium_src.chromium)
        self.assertNotIn('FROM_BRAVE_STORE', target_file.read_text())

        # Adding an upstream chromium change that should conflict with our
        # patch
        self.fake_chromium_src.write_and_stage_file(
            test_idl,
            target_file.read_text().replace('FROM_STORE',
                                            'FROM_STORE,\n    DELETED'),
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(
            'Added DELETED to developer_private.idl Location',
            self.fake_chromium_src.chromium)

        self.assertNotIn('FROM_BRAVE_STORE', target_file.read_text())
        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.CONFLICT)

    def test_apply_clean(self):
        test_idl = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_idl, """
                enum ExtensionType {
                    HOSTED_APP,
                    PLATFORM_APP,
                    LEGACY_PACKAGED_APP,
                    EXTENSION,
                    THEME,
                    SHARED_MODULE
                  };

                  enum Location {
                    FROM_STORE,
                    UNPACKED,
                    THIRD_PARTY,
                    INSTALLED_BY_DEFAULT,
                    UNKNOWN
                  };
                """, self.fake_chromium_src.chromium)

        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        # Let's create a patch for it
        target_file = self.fake_chromium_src.chromium / test_idl
        target_file.write_text(target_file.read_text().replace(
            'FROM_STORE', 'FROM_STORE,\n    FROM_BRAVE_STORE'))
        self.fake_chromium_src.run_update_patches()
        # clearing out our custom change so we can have upstream changes piling
        # to this file
        self.fake_chromium_src._run_git_command(
            ["checkout", "."], self.fake_chromium_src.chromium)

        # Adding an upstream chromium change to the file.
        self.fake_chromium_src.write_and_stage_file(
            test_idl,
            target_file.read_text() + """
                enum ViewType {
                  APP_WINDOW,
                  BACKGROUND_CONTENTS,
                  COMPONENT,
                  EXTENSION_BACKGROUND_PAGE,
                  EXTENSION_GUEST,
                  EXTENSION_POPUP,
                  EXTENSION_SERVICE_WORKER_BACKGROUND,
                  TAB_CONTENTS,
                  OFFSCREEN_DOCUMENT,
                  EXTENSION_SIDE_PANEL,
                  DEVELOPER_TOOLS
                };
              """, self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(
            'Added ViewType to developer_private.idl',
            self.fake_chromium_src.chromium)

        self.fake_chromium_src.write_and_stage_file(
            test_idl, """
                  enum ErrorType {
                    MANIFEST,
                    RUNTIME
                  };
                """ + target_file.read_text(), self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(
            'Added ErrorType to developer_private.idl',
            self.fake_chromium_src.chromium)

        self.assertNotIn('FROM_BRAVE_STORE', target_file.read_text())
        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.CLEAN)
        self.assertIn('FROM_BRAVE_STORE', target_file.read_text())

    def test_apply_broken(self):
        '''Tests the behavior when applying a broken patchfile.'''

        test_idl = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_idl, 'Just a test\nline1\line2\line3',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        target_file = self.fake_chromium_src.chromium / test_idl
        target_file.write_text(target_file.read_text() + 'last line\n')
        self.fake_chromium_src.run_update_patches()

        self.fake_chromium_src._run_git_command(
            ['checkout', '.'], self.fake_chromium_src.chromium)
        self.assertNotIn('last line', target_file.read_text())

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.CLEAN)
        self.assertIn('last line', target_file.read_text())

        self.fake_chromium_src._run_git_command(
            ['checkout', '--force', 'HEAD'], self.fake_chromium_src.chromium)
        self.assertNotIn('last line', target_file.read_text())

        # A simple strip over the contents of the patch should break it
        target_patch = (self.fake_chromium_src.brave /
                        self.fake_chromium_src.get_patchfile_path_for_source(
                            self.fake_chromium_src.chromium, test_idl))
        target_patch.write_text(target_patch.read_text().strip())

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.BROKEN)

    def test_apply_on_deleted(self):
        '''Tests the behavior when applying a patch to a deleted file.'''

        test_idl = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_idl, 'Just a test\nline1\line2\line3',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        target_file = self.fake_chromium_src.chromium / test_idl
        target_file.write_text(target_file.read_text() + 'last line\n')
        self.fake_chromium_src.run_update_patches()

        self.fake_chromium_src._run_git_command(
            ['checkout', '.'], self.fake_chromium_src.chromium)
        self.assertNotIn('last line', target_file.read_text())

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.CLEAN)
        self.assertIn('last line', target_file.read_text())

        self.fake_chromium_src._run_git_command(
            ['checkout', '--force', 'HEAD'], self.fake_chromium_src.chromium)
        self.assertNotIn('last line', target_file.read_text())

        # deleting file.
        self.fake_chromium_src.delete_file(test_idl,
                                           self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Delete developer_private.idl',
                                      self.fake_chromium_src.chromium)

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.DELETED)
        self.assertIsNone(result.rename_apply_status)

    def test_apply_on_deleted_with_whitespace_error(self):
        '''Tests DELETED status when stderr has whitespace warnings before the
        error: line (the bug that was fixed).'''

        test_idl = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_idl, 'Just a test\nline1\nline2\nline3\n',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        target_file = self.fake_chromium_src.chromium / test_idl
        # Trailing whitespace causes git to emit a warning line before the
        # error: line when applying against a deleted file.
        target_file.write_text(target_file.read_text() + 'last line   \n')
        self.fake_chromium_src.run_update_patches()

        self.fake_chromium_src._run_git_command(
            ['checkout', '.'], self.fake_chromium_src.chromium)
        self.assertNotIn('last line', target_file.read_text())

        # Sanity-check: patch applies cleanly before we delete the file.
        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.CLEAN)
        self.assertIn('last line', target_file.read_text())

        self.fake_chromium_src._run_git_command(
            ['checkout', '--force', 'HEAD'], self.fake_chromium_src.chromium)
        self.assertNotIn('last line', target_file.read_text())

        # Simulate upstream deletion of the file.
        self.fake_chromium_src.delete_file(test_idl,
                                           self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Delete developer_private.idl',
                                      self.fake_chromium_src.chromium)

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl),
            source=test_idl)
        result = patchfile.apply()
        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.DELETED)
        self.assertIsNone(result.rename_apply_status)

    def _setup_patch_for_rename_tests(self):
        """Shared setup: creates a patched file ready for rename scenarios.

        Returns (test_idl, renamed_idl, patchfile_path) after:
          - committing the original file,
          - making a brave patch that adds 'NEW_TYPE' after 'HOSTED_APP',
          - resetting the working tree to a clean state.
        """
        test_idl = Path('chrome/common/extensions/api/developer_private.idl')
        renamed_idl = Path('chrome/common/extensions/api/renamed_private.idl')
        chromium = self.fake_chromium_src.chromium

        self.fake_chromium_src.write_and_stage_file(
            test_idl, 'HOSTED_APP\nPLATFORM_APP\nFROM_STORE\n', chromium)
        self.fake_chromium_src.commit('Add developer_private.idl', chromium)

        (chromium / test_idl).write_text(
            (chromium / test_idl).read_text().replace('HOSTED_APP',
                                                      'HOSTED_APP\nNEW_TYPE'))
        self.fake_chromium_src.run_update_patches()

        self.fake_chromium_src._run_git_command(['checkout', '.'], chromium)
        self.assertNotIn('NEW_TYPE', (chromium / test_idl).read_text())

        patchfile_path = self.fake_chromium_src.get_patchfile_path_for_source(
            chromium, test_idl)
        return test_idl, renamed_idl, patchfile_path

    def test_apply_on_renamed_clean(self):
        """Patch applies cleanly to the renamed location."""
        test_idl, renamed_idl, patchfile_path = (
            self._setup_patch_for_rename_tests())
        chromium = self.fake_chromium_src.chromium

        # Rename the file upstream without modifying its content.
        (chromium / test_idl).rename(chromium / renamed_idl)
        self.fake_chromium_src._run_git_command(['add', '-A'], chromium)
        self.fake_chromium_src.commit(
            'Rename developer_private.idl to renamed_private.idl', chromium)

        result = Patchfile(path=patchfile_path, source=test_idl).apply()

        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.DELETED)
        self.assertEqual(result.rename_apply_status,
                         Patchfile.ApplyStatus.CLEAN)
        self.assertEqual(result.source_status.status_code, 'R')
        self.assertEqual(result.source_status.renamed_to,
                         renamed_idl.as_posix())
        self.assertIn('NEW_TYPE', (chromium / renamed_idl).read_text())

    def test_apply_on_renamed_conflict(self):
        """Patch conflicts at the renamed location due to an upstream change
        to the same area."""
        test_idl, renamed_idl, patchfile_path = (
            self._setup_patch_for_rename_tests())
        chromium = self.fake_chromium_src.chromium

        # Rename the file and introduce a conflicting change to the same line
        # that the brave patch modifies.
        renamed_path = chromium / renamed_idl
        renamed_path.parent.mkdir(parents=True, exist_ok=True)
        renamed_path.write_text(
            (chromium / test_idl).read_text().replace('HOSTED_APP',
                                                      'HOSTED_APP\nUPSTREAM'))
        (chromium / test_idl).unlink()
        self.fake_chromium_src._run_git_command(['add', '-A'], chromium)
        self.fake_chromium_src.commit(
            'Rename and modify developer_private.idl', chromium)

        result = Patchfile(path=patchfile_path, source=test_idl).apply()

        self.assertEqual(result.apply_status, Patchfile.ApplyStatus.DELETED)
        self.assertEqual(result.rename_apply_status,
                         Patchfile.ApplyStatus.CONFLICT)
        self.assertEqual(result.source_status.status_code, 'R')

    def test_source_from_brave(self):
        """Tests the source_from_brave method of Patchfile."""
        self.assertEqual(
            Patchfile(path=Path('patches/v8/build-android-gyp-dex.py.patch'),
                      source=Path('build/android/gyp/dex.py')).
            source_from_brave().as_posix(), '../v8/build/android/gyp/dex.py')
        self.assertEqual(
            Patchfile(path=Path('patches/build-android-gyp-dex.py.patch'),
                      source=Path('build/android/gyp/dex.py')).
            source_from_brave().as_posix(), '../build/android/gyp/dex.py')

        _devtools_patch = (
            'patches/third_party/devtools-frontend/src/'
            'front_end-panels-timeline-components-LiveMetricsView.ts.patch')
        _devtools_source = (
            'front_end/panels/timeline/components/LiveMetricsView.ts')
        self.assertEqual(
            Patchfile(
                path=Path(_devtools_patch),
                source=Path(_devtools_source)).source_from_brave().as_posix(),
            '../third_party/devtools-frontend/src/' + _devtools_source)

    def test_path_from_repo(self):
        """Tests the path_from_repo method of Patchfile."""
        self.assertEqual(
            Patchfile(
                path=Path('patches/v8/build-android-gyp-dex.py.patch'),
                source=Path(
                    'build/android/gyp/dex.py')).path_from_repo().as_posix(),
            '../brave/patches/v8/build-android-gyp-dex.py.patch')
        self.assertEqual(
            Patchfile(
                path=Path('patches/build-android-gyp-dex.py.patch'),
                source=Path(
                    'build/android/gyp/dex.py')).path_from_repo().as_posix(),
            'brave/patches/build-android-gyp-dex.py.patch')

    def test_get_last_commit_for_source(self):
        """Test get_last_commit_for_source method."""
        test_file = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_file, """
                enum ExtensionType {
                    HOSTED_APP,
                    PLATFORM_APP,
                    LEGACY_PACKAGED_APP,
                    EXTENSION,
                    THEME,
                    SHARED_MODULE
                  };
                """, self.fake_chromium_src.chromium)

        # Commit the file and get the short commit hash
        initial_commit_hash = self.fake_chromium_src.commit(
            'Add developer_private.idl', self.fake_chromium_src.chromium)[:7]

        # Create a patch for the file
        target_file = self.fake_chromium_src.chromium / test_file
        target_file.write_text(target_file.read_text().replace(
            'HOSTED_APP', 'HOSTED_APP,\n    NEW_TYPE'))
        self.fake_chromium_src.run_update_patches()

        # Add a few empty commits
        self.fake_chromium_src.commit_empty('Empty commit 1',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 2',
                                            self.fake_chromium_src.chromium)

        # Verify the last commit for the source matches the initial commit hash
        patchfile_path = self.fake_chromium_src.get_patchfile_path_for_source(
            self.fake_chromium_src.chromium, test_file)
        patchfile = Patchfile(path=patchfile_path, source=test_file)
        self.assertEqual(patchfile.get_last_commit_for_source(),
                         initial_commit_hash)

        # Delete the file and commit
        self.fake_chromium_src.delete_file(test_file,
                                           self.fake_chromium_src.chromium)
        delete_commit_hash = self.fake_chromium_src.commit(
            'Delete developer_private.idl',
            self.fake_chromium_src.chromium)[:7]

        # Add a few more empty commits
        self.fake_chromium_src.commit_empty('Empty commit 3',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 4',
                                            self.fake_chromium_src.chromium)

        # Get the patch file path
        patchfile = Patchfile(path=patchfile_path, source=test_file)

        # Verify the last commit for the source matches the delete commit hash
        self.assertEqual(patchfile.get_last_commit_for_source(),
                         delete_commit_hash)

    def test_get_source_removal_status(self):
        """Test get_source_removal_status for a deleted source file."""
        test_file = Path('chrome/common/extensions/api/developer_private.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_file, """
                enum ExtensionType {
                    HOSTED_APP,
                    PLATFORM_APP,
                    LEGACY_PACKAGED_APP,
                    EXTENSION,
                    THEME,
                    SHARED_MODULE
                  };
                """, self.fake_chromium_src.chromium)

        # Commit the file
        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        # Add a few empty commits
        self.fake_chromium_src.commit_empty('Empty commit 1',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 2',
                                            self.fake_chromium_src.chromium)

        # Create a patch for the file
        target_file = self.fake_chromium_src.chromium / test_file
        target_file.write_text(target_file.read_text().replace(
            'HOSTED_APP', 'HOSTED_APP,\n    NEW_TYPE'))
        self.fake_chromium_src.run_update_patches()

        # Delete the file and commit
        self.fake_chromium_src.delete_file(test_file,
                                           self.fake_chromium_src.chromium)
        delete_commit_hash = self.fake_chromium_src.commit(
            'Delete developer_private.idl', self.fake_chromium_src.chromium)

        # Get the patch file path
        patchfile_path = self.fake_chromium_src.get_patchfile_path_for_source(
            self.fake_chromium_src.chromium, test_file)
        patchfile = Patchfile(path=patchfile_path, source=test_file)

        # Add a few more empty commits
        self.fake_chromium_src.commit_empty('Empty commit 3',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 4',
                                            self.fake_chromium_src.chromium)

        # Verify the source removal status
        removal_status = patchfile.get_source_removal_status(
            delete_commit_hash)
        self.assertEqual(removal_status.status_code,
                         'D')  # 'D' indicates deletion
        self.assertEqual(removal_status.commit_hash, delete_commit_hash)
        self.assertIn('Delete developer_private.idl',
                      removal_status.commit_details)
        self.assertIsNone(removal_status.renamed_to)

    def test_get_source_rename_status(self):
        """Test get_source_removal_status for a renamed source file."""
        test_file = Path('chrome/common/extensions/api/developer_private.idl')
        renamed_file = Path('chrome/common/extensions/api/renamed_private.idl')

        # Write and commit the original file
        self.fake_chromium_src.write_and_stage_file(
            test_file, """
                enum ExtensionType {
                    HOSTED_APP,
                    PLATFORM_APP,
                    LEGACY_PACKAGED_APP,
                    EXTENSION,
                    THEME,
                    SHARED_MODULE
                  };
                """, self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add developer_private.idl',
                                      self.fake_chromium_src.chromium)

        # Add a few empty commits
        self.fake_chromium_src.commit_empty('Empty commit 1',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 2',
                                            self.fake_chromium_src.chromium)

        # Create a patch for the file
        target_file = self.fake_chromium_src.chromium / test_file
        target_file.write_text(target_file.read_text().replace(
            'HOSTED_APP', 'HOSTED_APP,\n    NEW_TYPE'))
        self.fake_chromium_src.run_update_patches()

        # Rename the file and commit
        (self.fake_chromium_src.chromium / test_file).rename(
            self.fake_chromium_src.chromium / renamed_file)
        self.fake_chromium_src._run_git_command(
            ['add', '-A'], self.fake_chromium_src.chromium)
        rename_commit_hash = self.fake_chromium_src.commit(
            'Rename developer_private.idl to renamed_private.idl',
            self.fake_chromium_src.chromium)

        # Get the patch file path
        patchfile_path = self.fake_chromium_src.get_patchfile_path_for_source(
            self.fake_chromium_src.chromium, test_file)
        patchfile = Patchfile(path=patchfile_path, source=test_file)

        # Add a few more empty commits
        self.fake_chromium_src.commit_empty('Empty commit 3',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 4',
                                            self.fake_chromium_src.chromium)

        # Verify the source rename status
        rename_status = patchfile.get_source_removal_status(rename_commit_hash)
        self.assertEqual(rename_status.status_code,
                         'R')  # 'R' indicates rename
        self.assertEqual(rename_status.commit_hash, rename_commit_hash)
        self.assertIn('Rename developer_private.idl to renamed_private.idl',
                      rename_status.commit_details)
        self.assertEqual(rename_status.renamed_to, renamed_file.as_posix())


if __name__ == "__main__":
    unittest.main()
