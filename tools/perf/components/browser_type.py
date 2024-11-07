# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
import os
import re
import shutil
import sys
from distutils.dir_util import copy_tree
from typing import List, Optional

from components.field_trials import FieldTrialsMode
from components.common_options import CommonOptions
from components.perf_test_utils import (DownloadArchiveAndUnpack, DownloadFile,
                                        GetProcessOutput, ToBravePlatformName,
                                        ToChromiumPlatformName)
from components.version import BraveVersion


def _GetBraveDownloadUrl(tag: str, filename: str) -> str:
  return ('https://github.com/brave/brave-browser/releases/download/' +
          f'{tag}/{filename}')


def _GetChromiumDownloadUrl(version: str, filename: str) -> str:
  return ('https://build-artifacts.brave.com/chromium-builds/' +
          f'{version}/{filename}')


def _GetChromeForTestingDownloadUrl(version: str, chrome_platform: str) -> str:
  return ('https://edgedl.me.gvt1.com/edgedl/chrome/chrome-for-testing/' +
          f'{str(version)}/{chrome_platform}/chrome-{chrome_platform}.zip')


def _DownloadWinInstallerAndExtract(out_dir: str, url: str,
                                    expected_install_path: str,
                                    binary_name: str) -> str:
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  installer_filename = os.path.join(out_dir, os.pardir, 'temp_installer.exe')
  DownloadFile(url, installer_filename)
  GetProcessOutput(
      [installer_filename, '--chrome-sxs', '--do-not-launch-chrome'], None,
      True)

  # Sometimes the binary is launched despite passing --do-not-launch-chrome.
  # Force kill it by taskkill.exe
  GetProcessOutput(['taskkill.exe', '/f', '/im', binary_name], None, False)

  if not os.path.exists(expected_install_path):
    raise RuntimeError(f'No files found in {expected_install_path}')

  full_version = None
  logging.info('Copy files to %s', out_dir)
  copy_tree(expected_install_path, out_dir)
  for file in os.listdir(expected_install_path):
    if re.match(r'\d+\.\d+\.\d+.\d+', file):
      assert (full_version is None)
      full_version = file
  assert (full_version is not None)
  logging.info('Detected version %s', full_version)
  setup_filename = os.path.join(expected_install_path, full_version,
                                'Installer', 'setup.exe')
  logging.info('Run uninstall')

  GetProcessOutput(
      [setup_filename, '--uninstall', '--force-uninstall', '--chrome-sxs'])
  shutil.rmtree(expected_install_path, True)

  return os.path.join(out_dir, binary_name)


def _FixUpUnpackedBrowser(out_dir: str):
  if sys.platform == 'darwin':
    GetProcessOutput(['xattr', '-cr', out_dir], check=True)


class BrowserType:
  _win_name: str
  _mac_name: str
  _channel: Optional[str] = None
  _extra_browser_args: List[str] = []
  _extra_benchmark_args: List[str] = []
  _report_as_reference = False

  def __init__(self, win_name: str, mac_name: str, channel: Optional[str],
               extra_browser_args: List[str], extra_benchmark_args: List[str],
               report_as_reference: bool):
    self._win_name = win_name
    self._mac_name = mac_name
    self._channel = channel
    self._extra_browser_args = extra_browser_args
    self._extra_benchmark_args = extra_benchmark_args
    self._report_as_reference = report_as_reference

  @property
  def is_brave(self) -> bool:
    return False

  @property
  def extra_browser_args(self) -> List[str]:
    return self._extra_browser_args

  @property
  def extra_benchmark_args(self) -> List[str]:
    return self._extra_benchmark_args

  @property
  def win_name(self) -> str:
    return self._win_name

  @property
  def mac_name(self) -> str:
    return self._mac_name

  @property
  def channel(self) -> Optional[str]:
    return self._channel

  @property
  def report_as_reference(self) -> bool:
    return self._report_as_reference

  def GetDefaultFieldTrials(self) -> FieldTrialsMode:
    return FieldTrialsMode.NO_TRIALS

  def DownloadBrowserBinary(self, url: Optional[str], version: BraveVersion,
                            out_dir: str, common_options: CommonOptions) -> str:
    raise NotImplementedError()

  def GetBinaryPath(self, target_os: str) -> str:
    if target_os == 'windows':
      return os.path.join(self.win_name + '.exe')

    if target_os == 'mac':
      if self.channel is not None:
        app_name = f'{self.mac_name} {self.channel}'
      else:
        app_name = self.mac_name
      return os.path.join(app_name + '.app', 'Contents', 'MacOS', app_name)

    raise RuntimeError(f'Unsupported platfrom {sys.platform}')


class BraveBrowserTypeImpl(BrowserType):

  def __init__(self, channel: str):
    super().__init__('brave', 'Brave Browser', channel, [], [], False)

  @property
  def is_brave(self) -> bool:
    return True

  def _GetWinInstallPath(self) -> str:
    app_name = 'Brave-Browser'
    if self.channel is not None:
      app_name += '-' + self.channel
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                        'BraveSoftware', app_name, 'Application')

  def DownloadBrowserBinary(self, url: Optional[str], version: BraveVersion,
                            out_dir: str, common_options: CommonOptions) -> str:
    if url is None and not version.is_tag:
      raise RuntimeError(
          f'Set the download url for revision {version.to_string()}')
    tag = version.last_tag
    target_os = common_options.target_os
    version_parts = tuple(map(int, tag[1:].split('.')))
    if (target_os == 'windows' and version_parts[0] == 1
        and version_parts[1] < 35):
      if url is None:
        url = _GetBraveDownloadUrl(
            tag, f'BraveBrowserStandaloneSilent{self.channel}Setup.exe')
      return _DownloadWinInstallerAndExtract(out_dir, url,
                                             self._GetWinInstallPath(),
                                             'brave.exe')
    if target_os == 'android':
      if url is None:
        url = _GetBraveDownloadUrl(tag, 'Bravearm64Universal.apk')
      apk_filename = os.path.join(out_dir, f'brave-{version.to_string()}.apk')
      DownloadFile(url, apk_filename)
      return apk_filename

    if url is None:
      brave_platform = ToBravePlatformName(target_os)
      url = _GetBraveDownloadUrl(tag, f'brave-{tag}-{brave_platform}.zip')
    DownloadArchiveAndUnpack(out_dir, url)
    _FixUpUnpackedBrowser(out_dir)

    return os.path.join(out_dir, self.GetBinaryPath(target_os))

  def GetDefaultFieldTrials(self) -> FieldTrialsMode:
    return FieldTrialsMode.GRIFFIN

class ChromiumBrowserTypeImpl(BrowserType):

  def __init__(self):
    super().__init__('chrome', 'Chromium', None, [], [], True)

  def DownloadBrowserBinary(self, url: Optional[str], version: BraveVersion,
                            out_dir: str, common_options: CommonOptions) -> str:
    target_os = common_options.target_os
    chromium_version_str = version.chromium_version.to_string()

    if target_os == 'android':
      filename = f'chromium-{chromium_version_str}-android-arm64.apk'
      if url is None:
        url = _GetChromiumDownloadUrl(chromium_version_str, filename)
      apk_path = os.path.join(out_dir, os.pardir, filename)
      DownloadFile(url, apk_path)
      return apk_path

    brave_platform_name = ToBravePlatformName(target_os)
    filename = f'chromium-{chromium_version_str}-{brave_platform_name}.zip'
    if url is None:
      url = _GetChromiumDownloadUrl(chromium_version_str, filename)
    DownloadArchiveAndUnpack(out_dir, url)
    _FixUpUnpackedBrowser(out_dir)
    return os.path.join(out_dir, self.GetBinaryPath(target_os))


class ChromeBrowserTypeImpl(BrowserType):

  def __init__(self, channel: str):
    super().__init__('chrome', 'Google Chrome', channel, [], [], True)

  def DownloadBrowserBinary(self, url: Optional[str], version: BraveVersion,
                            out_dir: str, common_options: CommonOptions) -> str:
    raise NotImplementedError()


class ChromeOfficialBrowserTypeImpl(ChromeBrowserTypeImpl):
  def _GetWinInstallPath(self) -> str:
    app_name = 'Chrome'
    if self.channel is not None:
      app_name += ' ' + self.channel
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local', 'Google',
                        app_name, 'Application')

  def DownloadBrowserBinary(self, url: Optional[str], version: BraveVersion,
                            out_dir: str, common_options: CommonOptions) -> str:
    if common_options.target_os == 'windows':
      if url is None:
        raise RuntimeError('Specify the binary url explicitly')
      return _DownloadWinInstallerAndExtract(out_dir, url,
                                             self._GetWinInstallPath(),
                                             'chrome.exe')
    raise NotImplementedError('OS is not supported')


class ChromeTestingBrowserTypeImpl(ChromeBrowserTypeImpl):
  def GetBinaryPath(self, target_os: str) -> str:
    chrome_platform = ToChromiumPlatformName(target_os)
    return os.path.join(f'chrome-{chrome_platform}',
                        super().GetBinaryPath(target_os))

  def DownloadBrowserBinary(self, url: Optional[str], version: BraveVersion,
                            out_dir: str, common_options: CommonOptions) -> str:
    chrome_platform = ToChromiumPlatformName(common_options.target_os)
    chromium_version_str = version.chromium_version.to_string()
    if url is None:
      url = _GetChromeForTestingDownloadUrl(chromium_version_str,
                                            chrome_platform)

    DownloadArchiveAndUnpack(out_dir, url)
    _FixUpUnpackedBrowser(out_dir)
    return os.path.join(out_dir, self.GetBinaryPath(common_options.target_os))


def ParseBrowserType(browser: str) -> BrowserType:
  if browser == 'chrome-official':
    return ChromeOfficialBrowserTypeImpl('SxS')

  if browser == 'chromium':
    return ChromiumBrowserTypeImpl()

  if browser == 'chrome':
    return ChromeTestingBrowserTypeImpl('for Testing')

  if browser == 'brave':
    return BraveBrowserTypeImpl('Nightly')
  if browser.startswith('custom'):
    return BrowserType(browser, browser, '', [], [], False)

  raise NotImplementedError(f"Unknown browser type {browser}")
