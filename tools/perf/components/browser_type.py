# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import os
import logging
import subprocess
import json
import shutil
import re

from typing import List, Optional

from urllib.request import urlopen
from io import BytesIO
from zipfile import ZipFile
from distutils.dir_util import copy_tree

from components import path_util
from components.perf_test_utils import GetProcessOutput


def _DownloadArchiveAndUnpack(output_directory: str, url: str) -> str:
  logging.info('Downloading archive %s', url)
  resp = urlopen(url)
  zipfile = ZipFile(BytesIO(resp.read()))
  zipfile.extractall(output_directory)
  return os.path.join(output_directory,
                      path_util.GetBinaryPath(output_directory))


def _DownloadWinInstallerAndExtract(out_dir: str, url: str,
                                    expected_install_path: str,
                                    binary_name: str) -> str:
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  installer_filename = os.path.join(out_dir, os.pardir, 'temp_installer.exe')
  logging.info('Downloading %s', url)
  f = urlopen(url)
  data = f.read()
  with open(installer_filename, 'wb') as output_file:
    output_file.write(data)
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

  def DownloadBrowserBinary(self, tag: str, out_dir: str) -> str:
    raise NotImplementedError()

  # pylint: disable=no-self-use
  def MakeFieldTrials(self, _tag: str, _out_dir: str,
                      _variations_repo_dir: Optional[str]) -> Optional[str]:
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

  @classmethod
  def _GetSetupDownloadUrl(cls, tag) -> str:
    return ('https://github.com/brave/brave-browser/releases/download/' +
            f'{tag}/BraveBrowserStandaloneSilentNightlySetup.exe')

  def _GetWinInstallPath(self) -> str:
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                        'BraveSoftware', 'Brave-Browser-' + self._channel,
                        'Application')

  @classmethod
  def _GetZipDownloadUrl(cls, tag) -> str:
    if sys.platform == 'win32':
      platform = 'win32-x64'
      return ('https://github.com/brave/brave-browser/releases/' +
              f'download/{tag}/brave-{tag}-{platform}.zip')
    raise NotImplementedError()

  def DownloadBrowserBinary(self, tag: str, out_dir: str) -> str:
    m = re.match(r'^v(\d+)\.(\d+)\.\d+$', tag)
    if not m:
      raise RuntimeError(f'Failed to parse tag "{tag}"')
    if (sys.platform == 'win32' and int(m.group(1)) == 1
        and int(m.group(2)) < 35):
      return _DownloadWinInstallerAndExtract(out_dir,
                                             self._GetSetupDownloadUrl(tag),
                                             self._GetWinInstallPath(),
                                             'brave.exe')

    return _DownloadArchiveAndUnpack(out_dir, self._GetZipDownloadUrl(tag))

  def MakeFieldTrials(self, tag: str, out_dir: str,
                      variations_repo_dir: Optional[str]) -> Optional[str]:
    if not self._use_field_trials:
      return None
    if not variations_repo_dir:
      raise RuntimeError('Set --variations-repo-dir to use field trials')
    return _MakeTestingFieldTrials(out_dir, tag, variations_repo_dir)

def _ParseVersion(version_string) -> List[str]:
  return version_string.split('.')

def _FetchTag(tag: str):
  tag_str = f'refs/tags/{tag}'
  args = ['git', 'fetch', 'origin', tag_str]
  GetProcessOutput(args, cwd=path_util.GetBraveDir(), check=True)
  return tag_str


def _GetBuildDate(tag: str) -> str:
  tag_str = _FetchTag(tag)
  _, output = GetProcessOutput(['git','show','-s','--format=%ci', tag_str],
                               cwd=path_util.GetBraveDir(), check=True)
  return output.rstrip()


def _MakeTestingFieldTrials(out_dir: str,
                            tag: str,
                            variations_repo_dir: str,
                            branch:str = 'production') -> str:
  chromium_version = _ParseVersion(_GetChromiumVersion(tag))
  assert re.match(r'v\d+\.\d+\.\d+', tag)
  combined_version = chromium_version[0] + '.' + tag[1:]
  logging.debug('combined_version %s', combined_version)
  target_path = os.path.join(out_dir, 'fieldtrial_testing_config.json')

  date = _GetBuildDate(tag)
  args = ['python3', 'seed/fieldtrials_testing_config_generator.py',
          f'--output={target_path}', f'--target-date={date}',
          f'--target-branch={branch}',
          f'--target-version={combined_version}', '--target-channel=NIGHTLY']
  GetProcessOutput(args, cwd=variations_repo_dir, check=True)
  return target_path


def _GetChromiumVersion(tag: str) -> str:
  tag_str = _FetchTag(tag)
  package_json = json.loads(
      subprocess.check_output(['git', 'show', f'{tag_str}:package.json'],
                              cwd=path_util.GetBraveDir()))
  return package_json['config']['projects']['chrome']['tag']

def _GetNearestChromiumUrl(tag: str) -> str:
  chrome_versions = {}
  with open(path_util.GetChromeReleasesJsonPath(), 'r') as config_file:
    chrome_versions = json.load(config_file)

  requested_version = _GetChromiumVersion(tag)
  logging.debug('Got requested_version: %s', requested_version)

  parsed_requested_version = _ParseVersion(requested_version)
  best_candidate = None
  for version in chrome_versions:
    parsed_version = _ParseVersion(version)
    if parsed_version[0] == parsed_requested_version[
        0] and parsed_version >= parsed_requested_version:
      if not best_candidate or best_candidate > parsed_version:
        best_candidate = parsed_version

  if best_candidate:
    string_version = '.'.join(map(str, best_candidate))
    logging.info('Use chromium version %s for requested %s', best_candidate,
                 requested_version)
    return chrome_versions[string_version]['url']

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

  def DownloadBrowserBinary(self, tag: str, out_dir: str) -> str:
    if sys.platform == 'win32':
      return _DownloadWinInstallerAndExtract(out_dir,
                                             _GetNearestChromiumUrl(tag),
                                             self._GetWinInstallPath(),
                                             'chrome.exe')

    raise NotImplementedError()


def ParseBrowserType(string_type: str) -> BrowserType:
  if string_type == 'chrome':
    return ChromeBrowserTypeImpl('chrome', 'SxS', ['--restore-last-session'],
                                 [], True)
  if string_type == 'chrome_no_trials':
    return ChromeBrowserTypeImpl('chrome', 'SxS', ['--restore-last-session'],
                                 ['--compatibility-mode=no-field-trials'],
                                 False)
  if string_type == 'brave':
    return BraveBrowserTypeImpl('brave', 'Nightly', True)
  if string_type == 'brave_no_trials':
    return BraveBrowserTypeImpl('brave', 'Nightly', False)
  if string_type.startswith('custom'):
    return BrowserType(string_type, [], [], False)

  raise NotImplementedError(f"Unknown browser type {string_type}")
