# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import json
import logging
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
from distutils.dir_util import copy_tree
from enum import Enum
from typing import List, Optional, Tuple
from urllib.request import urlopen

from lib.util import extract_zip

import components.path_util as path_util
from components.common_options import CommonOptions
from components.perf_test_utils import GetProcessOutput


def ToChromiumPlatform(target_os: str) -> str:
  if target_os == 'mac':
    return 'mac-arm64' if platform.processor() == 'arm' else 'mac-x64'
  if target_os == 'windows':
    return 'win64'
  if target_os == 'linux':
    return 'linux64'
  raise RuntimeError('Platform is not supported')


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


def _DownloadFile(url: str, output: str):
  logging.debug('Downloading %s', url)
  f = urlopen(url)
  data = f.read()
  with open(output, 'wb') as output_file:
    output_file.write(data)


def _GetBraveDownloadUrl(tag: BraveVersion, binary: str) -> str:
  return ('https://github.com/brave/brave-browser/releases/download/' +
          f'{tag}/{binary}')


def _GetChromeDownloadUrl(version: ChromiumVersion,
                          chrome_platform: str) -> str:
  return ('https://edgedl.me.gvt1.com/edgedl/chrome/chrome-for-testing/' +
          f'{str(version)}/{chrome_platform}/chrome-{chrome_platform}.zip')


def _DownloadArchiveAndUnpack(output_directory: str, url: str):
  f = tempfile.mktemp(dir=output_directory)
  _DownloadFile(url, f)
  extract_zip(f, output_directory)


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


def _FixUpUnpackedBrowser(out_dir: str):
  if sys.platform == 'darwin':
    GetProcessOutput(['xattr', '-cr', out_dir], check=True)


class FieldTrialsMode(Enum):
  NO_TRIALS = 1
  GRIFFIN = 2
  TESTING_FIELD_TRIALS = 3


class BrowserType:
  _name: str
  _mac_name: str
  _channel: str
  _extra_browser_args: List[str] = []
  _extra_benchmark_args: List[str] = []
  _field_trials_mode: FieldTrialsMode
  _report_as_reference = False

  def __init__(self, name: str, mac_name: str, channel: str,
               extra_browser_args: List[str], extra_benchmark_args: List[str],
               report_as_reference: bool, field_trials_mode: FieldTrialsMode):
    self._name = name
    self._mac_name = mac_name
    self._channel = channel
    self._extra_browser_args = extra_browser_args
    self._extra_benchmark_args = extra_benchmark_args
    self._report_as_reference = report_as_reference
    self._field_trials_mode = field_trials_mode

    if field_trials_mode != FieldTrialsMode.TESTING_FIELD_TRIALS:
      self.extra_browser_args.append('--disable-field-trial-config')
      self.extra_benchmark_args.append('--compatibility-mode=no-field-trials')

  @property
  def extra_browser_args(self) -> List[str]:
    return self._extra_browser_args

  @property
  def extra_benchmark_args(self) -> List[str]:
    return self._extra_benchmark_args

  @property
  def name(self) -> str:
    return self._name

  @property
  def mac_name(self) -> str:
    return self._mac_name

  @property
  def channel(self) -> str:
    return self._channel

  @property
  def report_as_reference(self) -> bool:
    return self._report_as_reference

  @property
  def field_trials_mode(self) -> FieldTrialsMode:
    return self._field_trials_mode

  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            common_options: CommonOptions) -> str:
    raise NotImplementedError()

  def MakeFieldTrials(self, tag: Optional[BraveVersion], artifacts_dir: str,
                      common_options: CommonOptions) -> Optional[str]:
    if self.field_trials_mode != FieldTrialsMode.GRIFFIN:
      return None
    if tag is None:
      raise RuntimeError('tag must be set to use Griffin trials')
    if not common_options.variations_repo_dir:
      raise RuntimeError('Set --variations-repo-dir to use Griffin trials')
    return _MakeTestingFieldTrials(artifacts_dir, tag,
                                   common_options.variations_repo_dir,
                                   'production', common_options.ci_mode)

  def GetBinaryPath(self, target_os: str) -> str:
    if sys.platform == 'win32':
      return os.path.join(self.name + '.exe')

    if target_os == 'mac':
      app_name = f'{self.mac_name} {self.channel}'
      return os.path.join(app_name + '.app', 'Contents', 'MacOS', app_name)

    raise RuntimeError(f'Unsupported platfrom {sys.platform}')


class BraveBrowserTypeImpl(BrowserType):
  def __init__(self, channel: str,
               field_trials_mode: Optional[FieldTrialsMode]):
    if field_trials_mode is None:
      field_trials_mode = FieldTrialsMode.GRIFFIN
    super().__init__('brave', 'Brave Browser', channel, [], [], False,
                     field_trials_mode)

  def _GetSetupDownloadUrl(self, tag: BraveVersion) -> str:
    return _GetBraveDownloadUrl(
        tag, f'BraveBrowserStandaloneSilent{self.channel}Setup.exe')

  def _GetWinInstallPath(self) -> str:
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                        'BraveSoftware', 'Brave-Browser-' + self.channel,
                        'Application')

  @classmethod
  def _GetZipDownloadUrl(cls, tag: BraveVersion, target_os: str) -> str:
    platform_name = None
    if target_os == 'windows':
      platform_name = 'win32-x64'
    if not platform_name:
      raise NotImplementedError()

    return _GetBraveDownloadUrl(tag, f'brave-{tag}-{platform_name}.zip')

  def _DownloadDmgAndExtract(self, tag: BraveVersion, out_dir: str):
    assert sys.platform == 'darwin'
    mac_platform = 'arm64' if platform.processor() == 'arm' else 'x64'
    dmg_name = f'Brave-Browser-{self._channel}-{mac_platform}.dmg'
    url = _GetBraveDownloadUrl(tag, dmg_name)
    logging.info('Downloading %s', url)
    f = urlopen(url)
    data = f.read()
    dmg_path = os.path.join(out_dir, dmg_name)

    _DownloadFile(_GetBraveDownloadUrl(tag, dmg_name), dmg_path)

    _, output = GetProcessOutput(
        ['hdiutil', 'attach', '-noautoopen', '-nobrowse', dmg_path], check=True)
    mount_path = output.rsplit('\t')[-1].rstrip()

    app_name = f'Brave Browser {self.channel}'
    GetProcessOutput(
        ['cp', '-R',
         os.path.join(mount_path, app_name + '.app'), out_dir],
        check=True)
    _FixUpUnpackedBrowser(out_dir)
    GetProcessOutput(['hdiutil', 'detach', mount_path], check=True)

  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            common_options: CommonOptions) -> str:
    target_os = common_options.target_os
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

    if target_os == 'mac':
      self._DownloadDmgAndExtract(tag, out_dir)
    else:
      _DownloadArchiveAndUnpack(out_dir,
                                self._GetZipDownloadUrl(tag, target_os))

    return os.path.join(out_dir, self.GetBinaryPath(target_os))


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
  def __init__(self, channel: str,
               field_trials_mode: Optional[FieldTrialsMode]):
    if field_trials_mode is None:
      field_trials_mode = FieldTrialsMode.TESTING_FIELD_TRIALS

    browser_args = []
    if field_trials_mode == FieldTrialsMode.TESTING_FIELD_TRIALS:
      browser_args = [
          '--variations-override-country=us', '--fake-variations-channel=stable'
      ]
    super().__init__('chrome', 'Google Chrome', channel, browser_args, [], True,
                     field_trials_mode)

  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            common_options: CommonOptions) -> str:
    raise NotImplementedError()


class ChromeOfficialBrowserTypeImpl(ChromeBrowserTypeImpl):
  def _GetWinInstallPath(self) -> str:
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local', 'Google',
                        'Chrome ' + self.channel, 'Application')

  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            common_options: CommonOptions) -> str:
    if common_options.target_os == 'windows':
      return _DownloadWinInstallerAndExtract(out_dir,
                                             _GetNearestChromiumUrl(tag),
                                             self._GetWinInstallPath(),
                                             'chrome.exe')

    raise NotImplementedError()


class ChromeTestingBrowserTypeImpl(ChromeBrowserTypeImpl):
  def DownloadBrowserBinary(self, tag: BraveVersion, out_dir: str,
                            common_options: CommonOptions) -> str:
    chrome_platform = ToChromiumPlatform(common_options.target_os)
    version = tag.to_chromium_version(common_options.ci_mode)
    url = _GetChromeDownloadUrl(version, chrome_platform)

    _DownloadArchiveAndUnpack(out_dir, url)
    _FixUpUnpackedBrowser(out_dir)
    return os.path.join(out_dir, f'chrome-{chrome_platform}',
                        self.GetBinaryPath(common_options.target_os))


def ParseFieldTrialMode(
    string_type: str) -> Tuple[str, Optional[FieldTrialsMode]]:
  if not '.' in string_type:
    return string_type, None
  [browser, subtype] = string_type.split('.', 2)

  if subtype == 'no-trials':
    return browser, FieldTrialsMode.NO_TRIALS
  if subtype == 'griffin':
    return browser, FieldTrialsMode.GRIFFIN
  if subtype == 'testing-field-trials':
    return browser, FieldTrialsMode.TESTING_FIELD_TRIALS
  raise RuntimeError('Bad browser type ' + string_type)


def ParseBrowserType(string_type: str) -> BrowserType:
  browser, field_trials_mode = ParseFieldTrialMode(string_type)
  if browser == 'chrome-official':
    return ChromeOfficialBrowserTypeImpl('SxS', field_trials_mode)

  if browser == 'chrome':
    return ChromeTestingBrowserTypeImpl('for Testing', field_trials_mode)

  if browser == 'brave':
    return BraveBrowserTypeImpl('Nightly', field_trials_mode)
  if string_type.startswith('custom'):
    return BrowserType(string_type, string_type, '', [], [], False,
                       FieldTrialsMode.NO_TRIALS)

  raise NotImplementedError(f"Unknown browser type {string_type}")
