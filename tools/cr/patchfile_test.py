#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path
from patchfile import Patchfile
import plaster
import repository
from repository import Repository

from test.fake_chromium_repo import FakeChromiumRepo


def _forget_loaded_repositories(test: unittest.TestCase) -> None:
    """Clear the repositories `load` memoised, for the test and after it.

    `Repositories.load` reads `.repositories.cfg` once, since a run only ever
    has the one. Tests write a different file per case, so each has to start
    and finish with nothing cached.
    """
    plaster.Repositories._instance = None
    test.addCleanup(setattr, plaster.Repositories, '_instance', None)


class PatchfileTest(unittest.TestCase):
    """Test the patchfile generation and application."""

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        self.fake_chromium_src.add_dep('v8')
        self.fake_chromium_src.add_dep('third_party/test1')
        self.fake_chromium_src.add_dep('third_party/devtools-frontend/src')
        self.addCleanup(self.fake_chromium_src.cleanup)
        _forget_loaded_repositories(self)

    def test_get_repository_from_patch_name(self):
        """Test the result of get_repository_from_patch_name"""
        dex_py = Path('build/android/gyp/dex.py')
        for repo in [
                self.fake_chromium_src.chromium,
                self.fake_chromium_src.chromium / 'v8',
                self.fake_chromium_src.chromium / 'third_party/test1'
        ]:
            self.fake_chromium_src.write_and_stage_file(
                dex_py, 'original\n', repo)
            self.fake_chromium_src.commit('Add dex.py', repo)
            (repo / dex_py).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        patchfile = Patchfile(
            path=Path("patches/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.get_repository_from_patch_name(),
                         repository.chromium)

        patchfile = Patchfile(
            path=Path("patches/v8/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.get_repository_from_patch_name(),
                         Repository(repository.chromium.root / 'v8'))

        patchfile = Patchfile(path=Path(
            "patches/third_party/test1/build-android-gyp-dex.py.patch"))
        self.assertEqual(
            patchfile.get_repository_from_patch_name(),
            Repository(repository.chromium.root / 'third_party/test1'))

    def test_source_name_from_patch_naming(self):
        """Test patched file name name heuristics."""
        dex_py = Path('build/android/gyp/dex.py')
        for repo in [
                self.fake_chromium_src.chromium,
                self.fake_chromium_src.chromium / 'v8',
                self.fake_chromium_src.chromium / 'third_party/test1'
        ]:
            self.fake_chromium_src.write_and_stage_file(
                dex_py, 'original\n', repo)
            self.fake_chromium_src.commit('Add dex.py', repo)
            (repo / dex_py).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        patchfile = Patchfile(
            path=Path("patches/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.source_name_from_patch_naming(),
                         "build/android/gyp/dex.py")

        patchfile = Patchfile(
            path=Path("patches/v8/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.source_name_from_patch_naming(),
                         "build/android/gyp/dex.py")

        patchfile = Patchfile(path=Path(
            "patches/third_party/test1/build-android-gyp-dex.py.patch"))
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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.CONFLICT)

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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.CONFLICT)

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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.CLEAN)
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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.CLEAN)
        self.assertIn('last line', target_file.read_text())

        self.fake_chromium_src._run_git_command(
            ['checkout', '--force', 'HEAD'], self.fake_chromium_src.chromium)
        self.assertNotIn('last line', target_file.read_text())

        # A simple strip over the contents of the patch should break it
        target_patch = (self.fake_chromium_src.brave /
                        self.fake_chromium_src.get_patchfile_path_for_source(
                            self.fake_chromium_src.chromium, test_idl))
        target_patch.write_text(target_patch.read_text().strip())

        # apply() reads the patch from disk at call time, so the same
        # Patchfile instance applies the now-broken patch.
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.BROKEN)

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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.CLEAN)
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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.DELETED)

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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.CLEAN)
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
                self.fake_chromium_src.chromium, test_idl))
        status = patchfile.apply()
        self.assertEqual(status, Patchfile.ApplyStatus.DELETED)

    def test_source_from_brave(self):
        """Tests the source_from_brave method of Patchfile."""
        dex_py = Path('build/android/gyp/dex.py')
        devtools_ts = Path(
            'front_end/panels/timeline/components/LiveMetricsView.ts')
        devtools_repo = (self.fake_chromium_src.chromium /
                         'third_party/devtools-frontend/src')
        for repo, source in [
            (self.fake_chromium_src.chromium, dex_py),
            (self.fake_chromium_src.chromium / 'v8', dex_py),
            (devtools_repo, devtools_ts),
        ]:
            self.fake_chromium_src.write_and_stage_file(
                source, 'original\n', repo)
            self.fake_chromium_src.commit(f'Add {source.name}', repo)
            (repo / source).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        self.assertEqual(
            Patchfile(path=Path('patches/v8/build-android-gyp-dex.py.patch')
                      ).source_from_brave(),
            repository.chromium.root / 'v8' / 'build/android/gyp/dex.py')
        self.assertEqual(
            Patchfile(path=Path(
                'patches/build-android-gyp-dex.py.patch')).source_from_brave(),
            repository.chromium.root / 'build/android/gyp/dex.py')

        _devtools_patch = ('patches/third_party/devtools-frontend/src/'
                           'front_end-panels-timeline-components-'
                           'LiveMetricsView.ts.patch')
        _devtools_source = (
            'front_end/panels/timeline/components/LiveMetricsView.ts')
        self.assertEqual(
            Patchfile(path=Path(_devtools_patch)).source_from_brave(),
            repository.chromium.root / 'third_party/devtools-frontend/src' /
            _devtools_source)

    def test_path_from_repo(self):
        """Tests the path_from_repo method of Patchfile."""
        dex_py = Path('build/android/gyp/dex.py')
        for repo in [
                self.fake_chromium_src.chromium,
                self.fake_chromium_src.chromium / 'v8'
        ]:
            self.fake_chromium_src.write_and_stage_file(
                dex_py, 'original\n', repo)
            self.fake_chromium_src.commit('Add dex.py', repo)
            (repo / dex_py).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        self.assertEqual(
            Patchfile(path=Path(
                'patches/v8/build-android-gyp-dex.py.patch')).path_from_repo(),
            Repository(repository.chromium.root / 'v8').to_brave() /
            'patches/v8/build-android-gyp-dex.py.patch')
        self.assertEqual(
            Patchfile(path=Path(
                'patches/build-android-gyp-dex.py.patch')).path_from_repo(),
            repository.chromium.to_brave() /
            'patches/build-android-gyp-dex.py.patch')

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
        patchfile = Patchfile(path=patchfile_path)
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
        patchfile = Patchfile(path=patchfile_path)

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
        patchfile = Patchfile(path=patchfile_path)

        # Add a few more empty commits
        self.fake_chromium_src.commit_empty('Empty commit 3',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 4',
                                            self.fake_chromium_src.chromium)

        # Verify the source removal status
        removal_status = patchfile.get_source_removal_status(
            delete_commit_hash)
        self.assertEqual(removal_status.status, 'D')  # 'D' indicates deletion
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
        patchfile = Patchfile(path=patchfile_path)

        # Add a few more empty commits
        self.fake_chromium_src.commit_empty('Empty commit 3',
                                            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit_empty('Empty commit 4',
                                            self.fake_chromium_src.chromium)

        # Verify the source rename status
        rename_status = patchfile.get_source_removal_status(rename_commit_hash)
        self.assertEqual(rename_status.status, 'R')  # 'R' indicates rename
        self.assertIn('Rename developer_private.idl to renamed_private.idl',
                      rename_status.commit_details)
        self.assertEqual(
            (self.fake_chromium_src.chromium /
             rename_status.renamed_to).resolve(),
            (self.fake_chromium_src.chromium / renamed_file).resolve())

    def test_plaster_set_when_file_exists(self):
        """Plaster path is set for a Chromium patch when the yaml exists."""
        test_file = Path('chrome/browser/foo.cc')
        self.fake_chromium_src.write_and_stage_file(
            test_file, 'original\n', self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add foo.cc',
                                      self.fake_chromium_src.chromium)
        (self.fake_chromium_src.chromium /
         test_file).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        plaster_path = (self.fake_chromium_src.brave /
                        'rewrite/chrome/browser/foo.cc.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('')

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_file))
        self.assertEqual(patchfile.plaster.resolve(), plaster_path.resolve())

    def test_plaster_none_when_file_missing(self):
        """Plaster is None for a Chromium patch when no yaml file exists."""
        test_file = Path('chrome/browser/foo.cc')
        self.fake_chromium_src.write_and_stage_file(
            test_file, 'original\n', self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add foo.cc',
                                      self.fake_chromium_src.chromium)
        (self.fake_chromium_src.chromium /
         test_file).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_file))
        self.assertIsNone(patchfile.plaster)

    def test_plaster_none_for_unlisted_repo(self):
        """Plaster is None for a repository `.repositories.cfg` omits."""
        test_file = Path('src/foo.cc')
        v8 = self.fake_chromium_src.chromium / 'v8'
        self.fake_chromium_src.write_and_stage_file(test_file, 'original\n',
                                                    v8)
        self.fake_chromium_src.commit('Add foo.cc', v8)
        (v8 / test_file).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        # A yaml that would match if the repository were listed. The fixture
        # lists `src` alone, so this patch is nobody's plaster.
        plaster_path = (self.fake_chromium_src.brave /
                        'rewrite/v8/src/foo.cc.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('')

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                v8, test_file))
        self.assertIsNone(patchfile.plaster)

    def test_plaster_set_for_listed_repo(self):
        """Plaster is found for a repository `.repositories.cfg` lists."""
        self.fake_chromium_src.set_patched_repositories('v8')
        test_file = Path('src/foo.cc')
        v8 = self.fake_chromium_src.chromium / 'v8'
        self.fake_chromium_src.write_and_stage_file(test_file, 'original\n',
                                                    v8)
        self.fake_chromium_src.commit('Add foo.cc', v8)
        (v8 / test_file).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        plaster_path = (self.fake_chromium_src.brave /
                        'rewrite/v8/src/foo.cc.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('')

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                v8, test_file))
        self.assertEqual(patchfile.plaster.resolve(), plaster_path.resolve())

    def test_repository_agrees_with_plaster_for_a_subrepo_patch(self):
        """The repository a patch names is the one its plaster resolves to.

        `Patchfile` reads the repository off the patch's own directory, while
        plaster reads it from `.repositories.cfg`. A patch plaster owns is
        handled by both, so the two have to land on the same repository and
        the same source within it.
        """
        self.fake_chromium_src.set_patched_repositories('v8')
        test_file = Path('src/foo.cc')
        v8 = self.fake_chromium_src.chromium / 'v8'
        self.fake_chromium_src.write_and_stage_file(test_file, 'original\n',
                                                    v8)
        self.fake_chromium_src.commit('Add foo.cc', v8)
        (v8 / test_file).write_text('original\nbrave_change\n')
        self.fake_chromium_src.run_update_patches()

        plaster_path = (self.fake_chromium_src.brave /
                        'rewrite/v8/src/foo.cc.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('')

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                v8, test_file))
        target = plaster.PlasterTarget.resolve(patchfile.plaster)
        self.assertEqual(patchfile.repository, target.repository)
        self.assertEqual(patchfile.source, Path(target.source))
        # And the patch plaster would write is the patch that was read.
        self.assertEqual(target.patch.resolve(),
                         (self.fake_chromium_src.brave /
                          patchfile.path).resolve())


class PatchfilePlasterApplyTest(unittest.TestCase):
    """Tests for `Patchfile.apply()` when a plaster file is associated.

    This is the path brockit drives during a lift: `apply_patches` reports a
    failed patch, and a plaster-managed one is re-applied from its plaster
    file rather than left conflicted.

    The scenarios are the same whichever repository holds the source, so they
    are written once here against chromium's own `src` and inherited by
    `SubrepositoryPlasterApplyTest` for a repository outside it. What differs
    is only which repository the git commands run in, and where the plaster
    and patch sit.
    """

    # The repository the patch lives in, relative to `src/`. Empty for
    # chromium's own `src`, whose patches sit at the root of `patches/`.
    _REPOSITORY = ''

    # The source patched, relative to that repository.
    _SOURCE_FILE = Path('chrome/browser/foo.cc')

    _BASE_CONTENT = 'void old_func() {}\nstatic int x = 0;\n'
    _BRAVE_CONTENT = 'void old_func() {}\nstatic int x = 1;\n'
    _UPSTREAM_CONTENT = 'void old_func() {}\nstatic int x = 2;\n'

    # count = 0 means "replace all matches, bypass count validation".
    _WORKING_YAML = ('substitutions:\n'
                     '  - description: Replace old_func\n'
                     '    regex:\n'
                     '      pattern: old_func\n'
                     '      replace: new_func\n'
                     '    count: 0\n')

    # No substitutions key -> missing required key -> PLASTER_BROKEN.
    _BROKEN_YAML = '# no substitution\n'

    def setUp(self):
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)
        _forget_loaded_repositories(self)
        self.repo_path = (self.fake_chromium_src.chromium / self._REPOSITORY)
        # `src` is there already and always listed; anything else has to be
        # created and named before plaster will patch it.
        if self._REPOSITORY:
            self.fake_chromium_src.add_repo(self._REPOSITORY)
            self.fake_chromium_src.set_patched_repositories(self._REPOSITORY)

    def _write_plaster_yaml(self, content: str) -> None:
        """Writes the plaster under its repository's prefix in `rewrite/`."""
        yaml_path = (self.fake_chromium_src.brave / 'rewrite' /
                     self._REPOSITORY / self._SOURCE_FILE.parent /
                     (self._SOURCE_FILE.name + '.yaml'))
        yaml_path.parent.mkdir(parents=True, exist_ok=True)
        yaml_path.write_text(content)

    def _commit_base_and_generate_patch(self) -> None:
        """Commits BASE_CONTENT and generates a brave patch (x=0 -> 1)."""
        self.fake_chromium_src.write_and_stage_file(self._SOURCE_FILE,
                                                    self._BASE_CONTENT,
                                                    self.repo_path)
        self.fake_chromium_src.commit(f'Add {self._SOURCE_FILE.name}',
                                      self.repo_path)

        (self.repo_path / self._SOURCE_FILE).write_text(self._BRAVE_CONTENT)
        self.fake_chromium_src.run_update_patches()
        self.fake_chromium_src._run_git_command(['checkout', '.'],
                                                self.repo_path)

    def _patchfile(self) -> Patchfile:
        """The `Patchfile` for the generated patch."""
        return Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.repo_path, self._SOURCE_FILE))

    def _setup_conflict_and_patchfile(self) -> Patchfile:
        """Generates a brave patch, then commits UPSTREAM_CONTENT (x=0 -> 2)
        so that applying the patch hits the conflict path in apply().

        Must be called after _write_plaster_yaml so the Patchfile constructor
        finds the YAML and sets the plaster field.
        """
        self._commit_base_and_generate_patch()
        self.fake_chromium_src.write_and_stage_file(self._SOURCE_FILE,
                                                    self._UPSTREAM_CONTENT,
                                                    self.repo_path)
        self.fake_chromium_src.commit('Upstream change', self.repo_path)
        return self._patchfile()

    def _setup_broken_and_patchfile(self) -> tuple[Patchfile, Path]:
        """Generates a valid brave patch.  Returns the Patchfile and the
        absolute path to the patch file on disk.

        Strip the patch file after this call to trigger the broken-patch path
        in apply().  Must be called after _write_plaster_yaml.
        """
        self._commit_base_and_generate_patch()
        patchfile = self._patchfile()
        return patchfile, self.fake_chromium_src.brave / patchfile.path

    def test_the_patch_is_recognised_as_plaster_managed(self):
        """The patch finds its plaster under its repository's prefix."""
        self._write_plaster_yaml(self._WORKING_YAML)
        patchfile = self._setup_conflict_and_patchfile()
        self.assertTrue(patchfile.has_plaster)
        self.assertEqual(patchfile.repository.relative_to_chromium,
                         Path(self._REPOSITORY))

    def test_apply_conflict_plaster_fixed(self):
        """Conflict path + working plaster -> PLASTER_FIXED."""
        self._write_plaster_yaml(self._WORKING_YAML)
        patchfile = self._setup_conflict_and_patchfile()
        self.assertEqual(patchfile.apply(),
                         Patchfile.ApplyStatus.PLASTER_FIXED)
        # The rewrite ran against the upstream content in the repository
        # holding it, so the source carries the substitution and not the
        # stale patch.
        self.assertEqual(
            (self.repo_path / self._SOURCE_FILE).read_text(),
            self._UPSTREAM_CONTENT.replace('old_func', 'new_func'))

    def test_apply_conflict_plaster_broken(self):
        """Conflict path + broken plaster -> PLASTER_BROKEN."""
        self._write_plaster_yaml(self._BROKEN_YAML)
        patchfile = self._setup_conflict_and_patchfile()
        self.assertEqual(patchfile.apply(),
                         Patchfile.ApplyStatus.PLASTER_BROKEN)

    def test_apply_broken_patch_plaster_fixed(self):
        """Broken patch path + working plaster -> PLASTER_FIXED."""
        self._write_plaster_yaml(self._WORKING_YAML)
        patchfile, patch_path = self._setup_broken_and_patchfile()
        patch_path.write_text(patch_path.read_text().strip())
        self.assertEqual(patchfile.apply(),
                         Patchfile.ApplyStatus.PLASTER_FIXED)

    def test_apply_broken_patch_plaster_broken(self):
        """Broken patch path + broken plaster -> PLASTER_BROKEN."""
        self._write_plaster_yaml(self._BROKEN_YAML)
        patchfile, patch_path = self._setup_broken_and_patchfile()
        patch_path.write_text(patch_path.read_text().strip())
        self.assertEqual(patchfile.apply(),
                         Patchfile.ApplyStatus.PLASTER_BROKEN)

    def test_apply_regenerates_the_patch_in_the_repository_directory(self):
        """The re-applied patch is rewritten where `apply_patches` reads it."""
        self._write_plaster_yaml(self._WORKING_YAML)
        patchfile = self._setup_conflict_and_patchfile()
        self.assertEqual(patchfile.apply(),
                         Patchfile.ApplyStatus.PLASTER_FIXED)
        patch_path = self.fake_chromium_src.brave / patchfile.path
        self.assertTrue(patch_path.exists())
        # Taken in the repository holding the source, so it names the source
        # the way that repository does, carrying no prefix of its own.
        self.assertIn(f'a/{self._SOURCE_FILE.as_posix()}',
                      patch_path.read_text())

    def test_source_from_brave_points_into_the_repository(self):
        """The path brockit reports for conflicts reaches the real source."""
        self._write_plaster_yaml(self._WORKING_YAML)
        patchfile = self._setup_conflict_and_patchfile()
        self.assertEqual(patchfile.source_from_brave().resolve(),
                         self.repo_path / self._SOURCE_FILE)


class SubrepositoryPlasterApplyTest(PatchfilePlasterApplyTest):
    """`PatchfilePlasterApplyTest` against a repository outside `src`.

    Every scenario is inherited; only the repository and the source within it
    change. Re-applying a plaster there means resetting the source and running
    the rewrite in that repository rather than in `src`.
    """

    _REPOSITORY = 'v8'
    _SOURCE_FILE = Path('src/codegen/compiler.cc')


if __name__ == "__main__":
    unittest.main()
