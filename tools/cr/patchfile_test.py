#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import PurePath, Path
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
            path=PurePath("patches/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.get_repository_from_patch_name(),
                         repository.chromium)

        patchfile = Patchfile(
            path=PurePath("patches/v8/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.get_repository_from_patch_name(),
                         Repository(self.fake_chromium_src.chromium / 'v8'))

        patchfile = Patchfile(path=PurePath(
            "patches/third_party/test1/build-android-gyp-dex.py.patch"))
        self.assertEqual(
            patchfile.get_repository_from_patch_name(),
            Repository(self.fake_chromium_src.chromium / 'third_party/test1'))

    def test_source_name_from_patch_naming(self):
        """Test patched file name name heuristics."""
        patchfile = Patchfile(
            path=PurePath("patches/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.source_name_from_patch_naming(),
                         "build/android/gyp/dex.py")

        patchfile = Patchfile(
            path=PurePath("patches/v8/build-android-gyp-dex.py.patch"))
        self.assertEqual(patchfile.source_name_from_patch_naming(),
                         "build/android/gyp/dex.py")

        patchfile = Patchfile(path=PurePath(
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

        # Let's create a patch for it
        target_file = self.fake_chromium_src.chromium / test_idl
        target_file.write_text(target_file.read_text().replace(
            'FROM_STORE', 'FROM_STORE,\n    FROM_BRAVE_STORE'))
        self.fake_chromium_src.run_update_patches()
        # clearing out our custom change so we can have upstream changes
        # piling to this file
        self.fake_chromium_src._run_git_command(
            ["checkout", "."], self.fake_chromium_src.chromium)
        self.assertFalse('FROM_BRAVE_STORE' in target_file.read_text())

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

        self.assertFalse('FROM_BRAVE_STORE' in target_file.read_text())
        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl))
        applied = patchfile.apply()
        self.assertEqual(applied.status, Patchfile.ApplyStatus.CONFLICT)

        # Check that in the case of conflict it returns an updated patchfile
        # instance with an updated source from git.
        self.assertTrue(applied.patch)
        self.assertEqual(applied.patch.path, patchfile.path)
        self.assertFalse(patchfile.source_from_git)
        self.assertTrue(applied.patch.source_from_git)
        self.assertEqual(applied.patch.source_from_git, str(test_idl))

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

        # Let's create a patch for it
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

        self.assertFalse('FROM_BRAVE_STORE' in target_file.read_text())
        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl))
        applied = patchfile.apply()
        self.assertEqual(applied.status, Patchfile.ApplyStatus.CLEAN)
        self.assertFalse(applied.patch)
        self.assertTrue('FROM_BRAVE_STORE' in target_file.read_text())

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
        self.assertFalse('last line' in target_file.read_text())

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl))
        applied = patchfile.apply()
        self.assertEqual(applied.status, Patchfile.ApplyStatus.CLEAN)
        self.assertFalse(applied.patch)
        self.assertTrue('last line' in target_file.read_text())

        self.fake_chromium_src._run_git_command(
            ['checkout', '--force', 'HEAD'], self.fake_chromium_src.chromium)
        self.assertFalse('last line' in target_file.read_text())

        # A simple strip over the contents of the patch should break it
        target_patch = (self.fake_chromium_src.brave /
                        self.fake_chromium_src.get_patchfile_path_for_source(
                            self.fake_chromium_src.chromium, test_idl))
        target_patch.write_text(target_patch.read_text().strip())

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl))
        applied = patchfile.apply()
        self.assertEqual(applied.status, Patchfile.ApplyStatus.BROKEN)
        self.assertFalse(applied.patch)

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
        self.assertFalse('last line' in target_file.read_text())

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl))
        applied = patchfile.apply()
        self.assertEqual(applied.status, Patchfile.ApplyStatus.CLEAN)
        self.assertFalse(applied.patch)
        self.assertTrue('last line' in target_file.read_text())

        self.fake_chromium_src._run_git_command(
            ['checkout', '--force', 'HEAD'], self.fake_chromium_src.chromium)
        self.assertFalse('last line' in target_file.read_text())

        # deleting file.
        self.fake_chromium_src.delete_file(test_idl,
                                           self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Delete developer_private.idl',
                                      self.fake_chromium_src.chromium)

        patchfile = Patchfile(
            path=self.fake_chromium_src.get_patchfile_path_for_source(
                self.fake_chromium_src.chromium, test_idl))
        applied = patchfile.apply()
        self.assertEqual(applied.status, Patchfile.ApplyStatus.DELETED)
        # A deleted patch should also provide an updated patchfile instance.
        self.assertTrue(applied.patch)
        self.assertEqual(applied.patch.path, patchfile.path)
        self.assertFalse(patchfile.source_from_git)
        self.assertTrue(applied.patch.source_from_git)
        self.assertEqual(applied.patch.source_from_git, str(test_idl))

    def test_source_from_brave(self):
        """Tests the source_from_brave method of Patchfile."""
        self.assertEqual(
            Patchfile(path=PurePath('patches/v8/build-android-gyp-dex.py.patch'
                                    )).source_from_brave(),
            '../v8/build/android/gyp/dex.py')
        self.assertEqual(
            Patchfile(path=PurePath(
                'patches/build-android-gyp-dex.py.patch')).source_from_brave(),
            '../build/android/gyp/dex.py')

    def test_path_from_repo(self):
        """Tests the path_from_repo method of Patchfile."""
        self.assertEqual(
            Patchfile(path=PurePath(
                'patches/v8/build-android-gyp-dex.py.patch')).path_from_repo(),
            '../brave/patches/v8/build-android-gyp-dex.py.patch')
        self.assertEqual(
            Patchfile(path=PurePath(
                'patches/build-android-gyp-dex.py.patch')).path_from_repo(),
            'brave/patches/build-android-gyp-dex.py.patch')


if __name__ == "__main__":
    unittest.main()
