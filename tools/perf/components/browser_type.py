#!/usr/bin/env python3
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
import sys
import re

from typing import List

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


def _DownloadWinInstallerAndExtract(out_dir: str,
                                    url: str,
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
  GetProcessOutput([installer_filename, '--chrome-sxs',
                    '--do-not-launch-chrome'], None, True)

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

  GetProcessOutput([setup_filename, '--uninstall',
                    '--force-uninstall', '--chrome-sxs'])
  shutil.rmtree(expected_install_path, True)

  return os.path.join(out_dir, binary_name)


class BrowserType:
  _name: str
  _extra_browser_args: List[str] = []
  _extra_benchmark_args: List[str] = []
  _report_as_reference = False

  def __init__(self,
               name: str,
               extra_browser_args: List[str],
               extra_benchmark_args: List[str],
               report_as_reference: bool):
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


class BraveBrowserTypeImpl(BrowserType):
  _channel: str

  def __init__(self,
               name: str,
               channel: str,
               extra_browser_args: List[str],
               extra_benchmark_args: List[str]):
    super().__init__(name, extra_browser_args, extra_benchmark_args, False)
    self._channel = channel

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
      return _DownloadWinInstallerAndExtract(
          out_dir,
          self._GetSetupDownloadUrl(tag),
          self._GetWinInstallPath(),
          'brave.exe')

    return _DownloadArchiveAndUnpack(out_dir, self._GetZipDownloadUrl(tag))


def _ParseVersion(version_string) -> List[str]:
  return version_string.split('.')


def _GetNearestChromiumUrl(tag: str) -> str:
  chrome_versions = {}
  with open(path_util.GetChromeReleasesJsonPath(), 'r') as config_file:
    chrome_versions = json.load(config_file)

  args = ['git', 'fetch', 'origin', (f'refs/tags/{tag}')]
  logging.debug('Run binary: %s', ' '.join(args))
  subprocess.check_call(args, cwd=path_util.GetBraveDir())
  package_json = json.loads(
      subprocess.check_output(['git', 'show', 'FETCH_HEAD:package.json'],
                              cwd=path_util.GetBraveDir()))
  requested_version = package_json['config']['projects']['chrome']['tag']
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

  def __init__(self,
               name: str,
               channel: str,
               extra_browser_args: List[str],
               extra_benchmark_args: List[str],
               report_as_reference: bool):
    super().__init__(name, extra_browser_args, extra_benchmark_args,
                     report_as_reference)
    self._channel = channel

  def _GetWinInstallPath(self) -> str:
    return os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                        'Google', 'Chrome ' + self._channel,
                        'Application')

  def DownloadBrowserBinary(self, tag: str, out_dir: str) -> str:
    if sys.platform == 'win32':
      return _DownloadWinInstallerAndExtract(
          out_dir,
          _GetNearestChromiumUrl(tag),
          self._GetWinInstallPath(),
          'chrome.exe')

    raise NotImplementedError()


def ParseBrowserType(string_type: str) -> BrowserType:
  if string_type == 'chrome':
    return ChromeBrowserTypeImpl('chrome', 'SxS',
                                 ['--restore-last-session'], [], True)
  if string_type == 'chrome_no_trials':
    return ChromeBrowserTypeImpl('chrome', 'SxS',
                                 ['--restore-last-session'],
                                 ['--compatibility-mode=no-field-trials'],
                                 False)
  if string_type == 'brave':
    return BraveBrowserTypeImpl('brave', 'Nightly', [],
                                ['--compatibility-mode=no-field-trials'])
  if string_type.startswith('custom'):
    return BrowserType(string_type, [], [], False)

  raise NotImplementedError(f"Unknown browser type {string_type}")
