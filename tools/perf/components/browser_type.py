# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import os
import logging
import subprocess
import json
import platform
import shutil
import re

from typing import List, Optional

from urllib.request import urlopen
from io import BytesIO
from zipfile import ZipFile
from distutils.dir_util import copy_tree

import components.path_util as path_util
from components.perf_test_utils import GetProcessOutput


class _BaseVersion:
  _version: List[int]

  def __eq__(self, other) -> bool:
    if not isinstance(other, _BaseVersion):
      return NotImplemented
    return self._version == other.version()

  def __lt__(self, other) -> bool:
    if not isinstance(other, _BaseVersion):
      return NotImplemented
    return self._version < other.version()

  def version(self) -> List[int]:
    return self._version


class BraveVersion(_BaseVersion):
  def __init__(self, tag: str) -> None:
    super().__init__()
    assert re.match(r'v\d+\.\d+\.\d+', tag)
    self._version = list(map(int, tag[1:].split('.')))

  def __str__(self) -> str:
    return 'v' + '.'.join(map(str, self._version))

  def to_chromium_version(self, ci_mode: bool) -> 'ChromiumVersion':
    if not ci_mode:
      _FetchTag(self)
    package_json = json.loads(
        subprocess.check_output(
            ['git', 'show', f'refs/tags/{self}:package.json'],
            cwd=path_util.GetBraveDir()))
    return ChromiumVersion(package_json['config']['projects']['chrome']['tag'])

  #Returns a version like 108.1.48.1
  def to_combined_version(self, ci_mode: bool) -> str:
    chromium_version = self.to_chromium_version(ci_mode)
    return f'{chromium_version.major()}.' + '.'.join(map(str, self._version))


class ChromiumVersion(_BaseVersion):
  def __init__(self, v: str) -> None:
    super().__init__()
    assert re.match(r'\d+\.\d+\.\d+\.\d+', v)
    self._version = list(map(int, v.split('.')))

  def __str__(self) -> str:
    return '.'.join(map(str, self._version))

  def major(self) -> int:
    return self._version[0]


def _GetBraveDownloadUrl(tag: BraveVersion, binary: str) -> str:
  return ('https://github.com/brave/brave-browser/releases/download/' +
          f'{tag}/{binary}')


def _DownloadArchiveAndUnpack(output_directory: str, url: str) -> str:
  logging.info('Downloading archive %s', url)
  resp = urlopen(url)
  zipfile = ZipFile(BytesIO(resp.read()))
  zipfile.extractall(output_directory)
  return os.path.join(output_directory,
                      path_util.GetBinaryPath(output_directory))


def _DownloadFile(url: str, target_file: str):
  logging.info('Downloading %s', url)
  f = urlopen(url)
  data = f.read()
  with open(target_file, 'wb') as output_file:
    output_file.write(data)


def _DownloadWinInstallerAndExtract(out_dir: str, url: str,
                                    expected_install_path: str,
                                    binary_name: str) -> str:
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  installer_filename = os.path.join(out_dir, os.pardir, 'temp_installer.exe')
  _DownloadFile(url, installer_filename)
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


class BrowserType:
  _name: str
  _extra_browser_args: List[str] = []
  _extra_benchmark_args: List[str] = []
  _report_as_reference = False

  def __init__(self, name: str, extra_browser_args: List[str],
               extra_benchmark_args: List[str], report_as_reference: bool):
    self._name = name
    self._extra_browser_args = extra_browser_args
    self._extra_benchmark_args = extra_benchmark_args
    self._report_as_reference = report_as_reference

  def GetExtraBrowserArgs(self) -> List[str]:
    return self._extra_browser_args

  def GetExtraBenchmarkArgs(self) -> List[str]:
    return self._extra_benchmark_args

  def GetName(self) -> str:
    return self._name

  def ReportAsReference(self) -> bool:
    return self._report_as_reference

  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            target_os: str) -> str:
    raise NotImplementedError()

  # pylint: disable=no-self-use
  def MakeFieldTrials(self, _tag: BraveVersion, _artifacts_dir: str,
                      _variations_repo_dir: Optional[str],
                      _ci_mode: bool) -> Optional[str]:
    return None


class BraveBrowserTypeImpl(BrowserType):
  _channel: str
  _use_field_trials: bool

  def __init__(self, name: str, channel: str, use_field_trials: bool):
    extra_benchmark_args = []
    if not use_field_trials:
      extra_benchmark_args.append('--compatibility-mode=no-field-trials')
    super().__init__(name, [], extra_benchmark_args, False)
    self._channel = channel
    self._use_field_trials = use_field_trials

  def _GetSetupDownloadUrl(self, tag: BraveVersion) -> str:
    return _GetBraveDownloadUrl(
        tag, f'BraveBrowserStandaloneSilent{self._channel}Setup.exe')

  def _GetWinInstallPath(self) -> str:
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                        'BraveSoftware', 'Brave-Browser-' + self._channel,
                        'Application')

  @classmethod
  def _GetZipDownloadUrl(cls, tag: BraveVersion, target_os: str) -> str:
    platform_name = None
    if target_os == 'windows':
      platform_name = 'win32-x64'
    if not platform_name:
      raise NotImplementedError()

    return _GetBraveDownloadUrl(tag, f'brave-{tag}-{platform_name}.zip')

  def _DownloadDmgAndExtract(self, tag: BraveVersion, out_dir: str) -> str:
    assert sys.platform == 'darwin'
    mac_platform = 'arm64' if platform.processor() == 'arm' else 'x64'
    dmg_name = f'Brave-Browser-{self._channel}-{mac_platform}.dmg'
    url = _GetBraveDownloadUrl(tag, dmg_name)
    logging.info('Downloading %s', url)
    f = urlopen(url)
    data = f.read()
    dmg_path = os.path.join(out_dir, dmg_name)
    with open(dmg_path, 'wb') as output_file:
      output_file.write(data)
    _, output = GetProcessOutput(
        ['hdiutil', 'attach', '-noautoopen', '-nobrowse', dmg_path], check=True)
    mount_path = output.rsplit('\t')[-1].rstrip()

    app_name = f'Brave Browser {self._channel}'
    GetProcessOutput(
        ['cp', '-R',
         os.path.join(mount_path, app_name + '.app'), out_dir],
        check=True)
    GetProcessOutput(['xattr', '-r', '-d', 'com.apple.quarantine', out_dir],
                     check=True)
    GetProcessOutput(['hdiutil', 'detach', mount_path], check=True)
    return os.path.join(out_dir, app_name + '.app', 'Contents', 'MacOS',
                        app_name)

  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            target_os: str) -> str:
    if target_os == 'mac':
      return self._DownloadDmgAndExtract(tag, out_dir)
    if (target_os == 'windows' and tag.version()[0] == 1
        and tag.version()[1] < 35):
      return _DownloadWinInstallerAndExtract(out_dir,
                                             self._GetSetupDownloadUrl(tag),
                                             self._GetWinInstallPath(),
                                             'brave.exe')
    if target_os == 'android':
      url = _GetBraveDownloadUrl(tag, 'BraveMonoarm64.apk')
      apk_filename = os.path.join(out_dir, os.pardir, 'BraveMonoarm64.apk')
      _DownloadFile(url, apk_filename)
      return apk_filename

    return _DownloadArchiveAndUnpack(out_dir,
                                     self._GetZipDownloadUrl(tag, target_os))

  def MakeFieldTrials(self, tag: BraveVersion, artifacts_dir: str,
                      variations_repo_dir: Optional[str],
                      ci_mode: bool) -> Optional[str]:
    if not self._use_field_trials:
      return None
    if not variations_repo_dir:
      raise RuntimeError('Set --variations-repo-dir to use field trials')
    return _MakeTestingFieldTrials(artifacts_dir, tag, variations_repo_dir,
                                   'production', ci_mode)


def _FetchTag(tag: BraveVersion):
  tag_str = f'refs/tags/{tag}'
  args = ['git', 'fetch', 'origin', tag_str]
  GetProcessOutput(args, cwd=path_util.GetBraveDir(), check=True)


def _GetBuildDate(tag: BraveVersion) -> str:
  tag_str = f'refs/tags/{tag}'
  _, output = GetProcessOutput(['git', 'show', '-s', '--format=%ci', tag_str],
                               cwd=path_util.GetBraveDir(),
                               check=True)
  build_date = output.rstrip().split('\n')[-1]
  logging.debug('Got build date %s', build_date)
  return build_date


def _MakeTestingFieldTrials(artifacts_dir: str, tag: BraveVersion,
                            variations_repo_dir: str, branch: str,
                            ci_mode: bool) -> str:
  combined_version = tag.to_combined_version(ci_mode)
  logging.debug('combined_version %s', combined_version)
  target_path = os.path.join(artifacts_dir, 'fieldtrial_testing_config.json')

  if not ci_mode:
    _FetchTag(tag)

  date = _GetBuildDate(tag)
  args = [
      path_util.GetVpython3Path(),
      'seed/fieldtrials_testing_config_generator.py', f'--output={target_path}',
      f'--target-date={date}', f'--target-branch={branch}',
      f'--target-version={combined_version}', '--target-channel=NIGHTLY'
  ]
  GetProcessOutput(args, cwd=variations_repo_dir, check=True)
  return target_path


def _GetNearestChromiumUrl(tag: BraveVersion) -> str:
  chrome_versions = {}
  with open(path_util.GetChromeReleasesJsonPath(), 'r') as config_file:
    chrome_versions = json.load(config_file)

  requested_version = tag.to_chromium_version(False)
  logging.debug('Got requested_version: %s', requested_version)

  best_candidate: Optional[ChromiumVersion] = None
  for version_str in chrome_versions:
    version = ChromiumVersion(version_str)
    if (version.major() == requested_version.major()
        and requested_version < version):
      if not best_candidate or version < best_candidate:
        best_candidate = version

  if best_candidate:
    logging.info('Use chromium version %s for requested %s', best_candidate,
                 requested_version)
    return chrome_versions[str(best_candidate)]['url']

  raise RuntimeError(f'No chromium version found for {requested_version}')


class ChromeBrowserTypeImpl(BrowserType):
  _channel: str

  def __init__(self, name: str, channel: str, extra_browser_args: List[str],
               extra_benchmark_args: List[str], report_as_reference: bool):
    super().__init__(name, extra_browser_args, extra_benchmark_args,
                     report_as_reference)
    self._channel = channel

  def _GetWinInstallPath(self) -> str:
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local', 'Google',
                        'Chrome ' + self._channel, 'Application')

  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            target_os: str) -> str:
    if target_os == 'windows':
      return _DownloadWinInstallerAndExtract(out_dir,
                                             _GetNearestChromiumUrl(tag),
                                             self._GetWinInstallPath(),
                                             'chrome.exe')

    raise NotImplementedError()


def ParseBrowserType(string_type: str) -> BrowserType:
  if string_type == 'chrome':
    return ChromeBrowserTypeImpl('chrome', 'SxS', [], [], True)
  if string_type == 'chrome_no_trials':
    return ChromeBrowserTypeImpl('chrome', 'SxS', [],
                                 ['--compatibility-mode=no-field-trials'],
                                 False)
  if string_type == 'brave':
    return BraveBrowserTypeImpl('brave', 'Nightly', True)
  if string_type == 'brave_no_trials':
    return BraveBrowserTypeImpl('brave', 'Nightly', False)
  if string_type.startswith('custom'):
    return BrowserType(string_type, [], [], False)

  raise NotImplementedError(f"Unknown browser type {string_type}")
