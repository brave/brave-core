#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import unittest
import os
import upload
import shutil
from githubmock import Repo, Release, Asset
from mock import MagicMock

dirname = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(dirname, '..'))


class TestGetBravePackages(unittest.TestCase):
    get_pkgs_dir = os.path.join(os.path.dirname(
        os.path.realpath(__file__)), 'test_get_pkgs')
    _is_setup = False

    def setUp(self):
        if not self._is_setup:
            shutil.rmtree(self.get_pkgs_dir)
            os.mkdir(self.get_pkgs_dir)
            android_dir = os.path.join(self.get_pkgs_dir, 'android')
            darwin_dir = os.path.join(self.get_pkgs_dir, 'darwin')
            linux_dir = os.path.join(self.get_pkgs_dir, 'linux')
            windows_dir = os.path.join(self.get_pkgs_dir, 'win32')
            if not os.path.isdir(android_dir):
                os.mkdir(android_dir)
            if not os.path.isdir(darwin_dir):
                os.mkdir(darwin_dir)
            if not os.path.isdir(linux_dir):
                os.mkdir(linux_dir)
            if not os.path.isdir(windows_dir):
                os.mkdir(windows_dir)
            for arch in ['Modernarm', 'Modernx86', 'Monoarm', 'Monoarm64', 'Monox64', 'Monox86', 'arm', 'x86']:
                name_android = 'Brave' + arch + '.apk'
                with open(os.path.join(android_dir, name_android), 'w') as f:
                    f.write(name_android + '\n')
            for channel in ['nightly', 'dev', 'beta', 'release']:
                channel_capitalized = channel.capitalize()
                channel_dashed = '' if (
                    channel == 'release') else ('-' + channel)
                name_darwin = 'Brave Browser ' + channel_capitalized + '.dmg'
                name_darwin_pkg = 'Brave Browser ' + channel_capitalized + '.pkg'
                name_linux_deb = 'brave-browser' + channel_dashed + '_1.5.8_amd64.deb'
                name_linux_rpm = 'brave-browser' + channel_dashed + '-1.5.8-1.x86_64.rpm'
                name_windows_installer = 'brave_installer.exe'
                with open(os.path.join(darwin_dir, name_darwin), 'w') as f:
                    f.write(name_darwin + '\n')
                with open(os.path.join(darwin_dir, name_darwin_pkg), 'w') as f:
                    f.write(name_darwin_pkg + '\n')
                with open(os.path.join(linux_dir, name_linux_deb), 'w') as f:
                    f.write(name_linux_deb + '\n')
                with open(os.path.join(linux_dir, name_linux_rpm), 'w') as f:
                    f.write(name_linux_rpm + '\n')
                with open(os.path.join(windows_dir, name_windows_installer), 'w') as f:
                    f.write(name_windows_installer + '\n')
                for mode in ['Stub', 'Silent', 'Untagged', 'Standalone', 'StandaloneSilent', 'StandaloneUntagged']:
                    name = 'BraveBrowser{}{}Setup_70_1_5_8.exe'.format(
                        mode if mode not in 'Stub' else '', channel_capitalized if channel != 'release' else '')
                    name32 = 'BraveBrowser{}{}Setup32_70_1_5_8.exe'.format(
                        mode if mode not in 'Stub' else '', channel_capitalized if channel != 'release' else '')
                    with open(os.path.join(windows_dir, name), 'w') as f:
                        f.write(name + '\n')
                    with open(os.path.join(windows_dir, name32), 'w') as f:
                        f.write(name32 + '\n')
        self.__class__._is_setup = True

    def test_only_returns_android_packages(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', ''))
        self.assertEquals(pkgs,
                          sorted(['BraveModernarm.apk',
                                  'BraveModernx86.apk',
                                  'BraveMonoarm.apk',
                                  'BraveMonoarm64.apk',
                                  'BraveMonox64.apk',
                                  'BraveMonox86.apk',
                                  'Bravearm.apk',
                                  'Bravex86.apk']))

    def test_only_returns_android_packages_arm(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'arm'))
        self.assertEquals(pkgs,
                          sorted(['BraveModernarm.apk',
                                  'BraveMonoarm.apk',
                                  'Bravearm.apk']))

    def test_only_returns_android_packages_arm_classic(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'arm', 'classic'))
        self.assertEquals(pkgs,
                          sorted(['Bravearm.apk']))

    def test_only_returns_android_packages_arm_modern(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'arm', 'modern'))
        self.assertEquals(pkgs,
                          sorted(['BraveModernarm.apk']))

    def test_only_returns_android_packages_arm_mono(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'arm', 'mono'))
        self.assertEquals(pkgs,
                          sorted(['BraveMonoarm.apk']))

    def test_only_returns_android_packages_arm64(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'arm64'))
        self.assertEquals(pkgs,
                          sorted(['BraveMonoarm64.apk']))

    def test_only_returns_android_packages_x86(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'x86'))
        self.assertEquals(pkgs,
                          sorted(['BraveModernx86.apk',
                                  'BraveMonox86.apk',
                                  'Bravex86.apk']))

    def test_only_returns_android_packages_x86_classic(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'x86', 'classic'))
        self.assertEquals(pkgs,
                          sorted(['Bravex86.apk']))

    def test_only_returns_android_packages_x86_modern(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'x86', 'modern'))
        self.assertEquals(pkgs,
                          sorted(['BraveModernx86.apk']))

    def test_only_returns_android_packages_x86_mono(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'x86', 'mono'))
        self.assertEquals(pkgs,
                          sorted(['BraveMonox86.apk']))

    def test_only_returns_android_packages_x64(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           'android'),
                                              'nightly', '1.5.8', 'android', 'x64'))
        self.assertEquals(pkgs,
                          sorted(['BraveMonox64.apk']))

    def test_only_returns_nightly_darwin_packages(self):
        upload.PLATFORM = 'darwin'
        pkgs = list(upload.get_brave_packages(os.path.join(
            self.get_pkgs_dir, upload.PLATFORM), 'nightly', '1.5.8', 'darwin'))
        self.assertEquals(pkgs, sorted(
            ['Brave-Browser-Nightly.dmg', 'Brave-Browser-Nightly.pkg']))

    def test_only_returns_dev_darwin_package(self):
        upload.PLATFORM = 'darwin'
        pkgs = list(upload.get_brave_packages(os.path.join(
            self.get_pkgs_dir, upload.PLATFORM), 'dev', '1.5.8', 'darwin'))
        self.assertEquals(pkgs, sorted(
            ['Brave-Browser-Dev.dmg', 'Brave-Browser-Dev.pkg']))

    def test_only_returns_beta_darwin_package(self):
        upload.PLATFORM = 'darwin'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM), 'beta', '1.5.8', 'darwin'))
        self.assertEquals(pkgs, sorted(
            ['Brave-Browser-Beta.dmg', 'Brave-Browser-Beta.pkg']))

    def test_only_returns_release_darwin_packages(self):
        upload.PLATFORM = 'darwin'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM),
            'release', '1.5.8', 'darwin'))
        self.assertEquals(pkgs, sorted(
            ['Brave-Browser.dmg', 'Brave-Browser.pkg']))

    def test_only_returns_nightly_linux_packages(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           upload.PLATFORM),
                                              'nightly', '1.5.8', 'darwin'))
        self.assertEquals(pkgs,
                          sorted(['brave-browser-nightly-1.5.8-1.x86_64.rpm',
                                  'brave-browser-nightly_1.5.8_amd64.deb']))

    def test_only_returns_dev_linux_packages(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           upload.PLATFORM),
                                              'dev', '1.5.8', 'linux'))
        self.assertEquals(pkgs,
                          sorted(['brave-browser-dev-1.5.8-1.x86_64.rpm',
                                  'brave-browser-dev_1.5.8_amd64.deb']))

    def test_only_returns_beta_linux_packages(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           upload.PLATFORM),
                                              'beta', '1.5.8', 'linux'))
        self.assertEquals(pkgs,
                          sorted(['brave-browser-beta-1.5.8-1.x86_64.rpm',
                                  'brave-browser-beta_1.5.8_amd64.deb']))

    def test_only_returns_release_linux_packages(self):
        upload.PLATFORM = 'linux'
        pkgs = list(upload.get_brave_packages(os.path.join(self.get_pkgs_dir,
                                                           upload.PLATFORM),
                                              'release', '1.5.8', 'linux'))
        self.assertEquals(pkgs,
                          sorted(['brave-browser-1.5.8-1.x86_64.rpm',
                                  'brave-browser_1.5.8_amd64.deb']))

    def test_only_returns_nightly_win_x64_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(os.path.join(
            self.get_pkgs_dir, upload.PLATFORM), 'nightly', '1.5.8', 'win32'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-x64.exe',
                          'BraveBrowserNightlySetup.exe',
                          'BraveBrowserStandaloneNightlySetup.exe',
                          'BraveBrowserStandaloneSilentNightlySetup.exe',
                          'BraveBrowserStandaloneUntaggedNightlySetup.exe',
                          'BraveBrowserUntaggedNightlySetup.exe']))

    def test_only_returns_nightly_win_ia32_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM), 'nightly', '1.5.8', 'win32', 'ia32'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-ia32.exe',
                          'BraveBrowserNightlySetup32.exe',
                          'BraveBrowserStandaloneNightlySetup32.exe',
                          'BraveBrowserStandaloneSilentNightlySetup32.exe',
                          'BraveBrowserStandaloneUntaggedNightlySetup32.exe',
                          'BraveBrowserUntaggedNightlySetup32.exe']))

    def test_only_returns_nightly_win_x86_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM), 'nightly', '1.5.8', 'win32', 'x86'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-ia32.exe',
                          'BraveBrowserNightlySetup32.exe',
                          'BraveBrowserStandaloneNightlySetup32.exe',
                          'BraveBrowserStandaloneSilentNightlySetup32.exe',
                          'BraveBrowserStandaloneUntaggedNightlySetup32.exe',
                          'BraveBrowserUntaggedNightlySetup32.exe']))

    def test_only_returns_dev_win_x64_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(os.path.join(
            self.get_pkgs_dir, upload.PLATFORM), 'dev', '1.5.8', 'win32'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-x64.exe',
                          'BraveBrowserDevSetup.exe',
                          'BraveBrowserStandaloneDevSetup.exe',
                          'BraveBrowserStandaloneSilentDevSetup.exe',
                          'BraveBrowserStandaloneUntaggedDevSetup.exe',
                          'BraveBrowserUntaggedDevSetup.exe']))

    def test_only_returns_dev_win_ia32_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM), 'dev', '1.5.8', 'win32', 'ia32'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-ia32.exe',
                          'BraveBrowserDevSetup32.exe',
                          'BraveBrowserStandaloneDevSetup32.exe',
                          'BraveBrowserStandaloneSilentDevSetup32.exe',
                          'BraveBrowserStandaloneUntaggedDevSetup32.exe',
                          'BraveBrowserUntaggedDevSetup32.exe']))

    def test_only_returns_dev_win_x86_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM), 'dev', '1.5.8', 'win32', 'x86'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-ia32.exe',
                          'BraveBrowserDevSetup32.exe',
                          'BraveBrowserStandaloneDevSetup32.exe',
                          'BraveBrowserStandaloneSilentDevSetup32.exe',
                          'BraveBrowserStandaloneUntaggedDevSetup32.exe',
                          'BraveBrowserUntaggedDevSetup32.exe']))

    def test_only_returns_beta_win_x64_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM),
            'beta', '1.5.8', 'win32'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-x64.exe',
                          'BraveBrowserBetaSetup.exe',
                          'BraveBrowserStandaloneBetaSetup.exe',
                          'BraveBrowserStandaloneSilentBetaSetup.exe',
                          'BraveBrowserStandaloneUntaggedBetaSetup.exe',
                          'BraveBrowserUntaggedBetaSetup.exe']))

    def test_only_returns_beta_win_ia32_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM),
            'beta', '1.5.8', 'win32', 'ia32'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-ia32.exe',
                          'BraveBrowserBetaSetup32.exe',
                          'BraveBrowserStandaloneBetaSetup32.exe',
                          'BraveBrowserStandaloneSilentBetaSetup32.exe',
                          'BraveBrowserStandaloneUntaggedBetaSetup32.exe',
                          'BraveBrowserUntaggedBetaSetup32.exe']))

    def test_only_returns_beta_win_x86_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM),
            'beta', '1.5.8', 'win32', 'x86'))
        self.assertEquals(
            pkgs, sorted(['brave_installer-ia32.exe',
                          'BraveBrowserBetaSetup32.exe',
                          'BraveBrowserStandaloneBetaSetup32.exe',
                          'BraveBrowserStandaloneSilentBetaSetup32.exe',
                          'BraveBrowserStandaloneUntaggedBetaSetup32.exe',
                          'BraveBrowserUntaggedBetaSetup32.exe']))

    def test_only_returns_release_win_x64_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM),
            'release', '1.5.8', 'win32'))
        self.assertEquals(pkgs, sorted(['brave_installer-x64.exe',
                                        'BraveBrowserSetup.exe',
                                        'BraveBrowserStandaloneSetup.exe',
                                        'BraveBrowserStandaloneSilentSetup.exe',
                                        'BraveBrowserStandaloneUntaggedSetup.exe',
                                        'BraveBrowserUntaggedSetup.exe']))

    def test_only_returns_release_win_ia32_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM),
            'release', '1.5.8', 'win32', 'ia32'))
        self.assertEquals(pkgs, sorted(['brave_installer-ia32.exe',
                                        'BraveBrowserSetup32.exe',
                                        'BraveBrowserStandaloneSetup32.exe',
                                        'BraveBrowserStandaloneSilentSetup32.exe',
                                        'BraveBrowserStandaloneUntaggedSetup32.exe',
                                        'BraveBrowserUntaggedSetup32.exe']))

    def test_only_returns_release_win_x86_packages(self):
        upload.PLATFORM = 'win32'
        pkgs = list(upload.get_brave_packages(
            os.path.join(self.get_pkgs_dir, upload.PLATFORM),
            'release', '1.5.8', 'win32', 'x86'))
        self.assertEquals(pkgs, sorted(['brave_installer-ia32.exe',
                                        'BraveBrowserSetup32.exe',
                                        'BraveBrowserStandaloneSetup32.exe',
                                        'BraveBrowserStandaloneSilentSetup32.exe',
                                        'BraveBrowserStandaloneUntaggedSetup32.exe',
                                        'BraveBrowserUntaggedSetup32.exe']))

# get an existing release (in draft status) from GitHub given a tag name


class TestGetDraft(unittest.TestCase):
    def setUp(self):
        self.repo = Repo()

    @unittest.skip("TODO: mbacchi fix test")
    def test_returns_existing_draft(self):
        self.repo.releases._releases = [{'tag_name': 'test', 'draft': True}]
        self.assertEquals(upload.get_release(self.repo,
                                             'test')['tag_name'], 'test', False)

    @unittest.skip("TODO: mbacchi fix test")
    def test_fails_on_existing_release(self):
        self.repo.releases._releases = [
            {'tag_name': 'test', 'draft': False, 'allow_published_release_updates': False}]
        self.assertRaises(UserWarning, upload.get_release,
                          self.repo, 'test', False)

    @unittest.skip("TODO: mbacchi fix test")
    def test_returns_none_on_new_draft(self):
        self.repo.releases._releases = [{'tag_name': 'old', 'draft': False}]
        upload.get_release(self.repo, 'new', False)
        self.assertEquals(upload.get_release(self.repo, 'test'), None)


# uploading a single file to GitHub
class TestUploadBrave(unittest.TestCase):
    def setUp(self):
        self.repo = Repo()
        self.file_path = os.path.join(os.path.dirname(
            os.path.realpath(__file__)),
            'test_get_pkgs',
            'win32',
            'BraveBrowserSetup.exe')
        self.release = Release()
        self.release.id = 1
        self.release.tag_name = 'release-tag-here'
        self.asset = Asset(1, 'BraveBrowserSetup.exe')
        self.release.assets._assets.append(self.asset)
        self.repo.releases._releases = [self.release]
        self.repo.releases.assets = self.release.assets

        self._old_upload_io_to_github = upload.upload_io_to_github
        upload.upload_io_to_github = MagicMock()

        self._old_delete_file = upload.delete_file
        upload.delete_file = MagicMock()

    def tearDown(self):
        upload.upload_io_to_github = self._old_upload_io_to_github
        upload.delete_file = self._old_delete_file

    def test_calls_delete_if_asset_already_exists(self):
        upload.upload_brave(self.repo, self.release, self.file_path)
        self.asset.delete.assert_called()

    def test_does_not_call_delete_if_asset_not_present(self):
        empty_repo = Repo()
        upload.upload_brave(empty_repo, self.release, self.file_path)
        self.asset.delete.assert_not_called()

    def test_does_not_force_delete_before_upload_when_force_false(self):
        upload.upload_brave(self.repo, self.release, self.file_path)
        upload.delete_file.assert_not_called()

    def test_force_delete_before_upload_when_force_true(self):
        upload.upload_brave(self.repo, self.release,
                            self.file_path, None, True)
        upload.delete_file.assert_called_with(
            self.repo, self.release, self.release.assets._assets[0].name)

    def test_force_delete_before_upload_when_force_true_with_filename_override(self):
        fake_filename = 'fake-name-here'
        upload.upload_brave(self.repo, self.release,
                            self.file_path, fake_filename, True)
        upload.delete_file.assert_called_with(
            self.repo, self.release, fake_filename)

    def test_calls_upload(self):
        upload.upload_brave(self.repo, self.release, self.file_path)
        upload.upload_io_to_github.assert_called()

        args, kwargs = upload.upload_io_to_github.call_args
        self.assertTrue(args[0] == self.repo)
        self.assertTrue(args[2] == self.release.assets._assets[0].name)
        self.assertTrue(args[4] == 'application/zip')

    def test_calls_upload_with_filename_override(self):
        fake_filename = 'fake-name-here'
        upload.upload_brave(self.repo, self.release,
                            self.file_path, fake_filename)
        upload.upload_io_to_github.assert_called()

        args, kwargs = upload.upload_io_to_github.call_args
        self.assertTrue(args[0] == self.repo)
        self.assertTrue(args[2] == fake_filename)
        self.assertTrue(args[4] == 'application/zip')

    # NOTE: retries tested in test_helpers.py with TestRetryFunc

    def test_calls_uploads_checksum(self):
        upload.upload_brave(self.repo, self.release, self.file_path)

    # TODO: test `armv7l` code path


if __name__ == '__main__':
    print unittest.main()
