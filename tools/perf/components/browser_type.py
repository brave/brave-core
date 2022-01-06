#!/usr/bin/env python3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import os
import logging

import subprocess

from typing import List
import json

from components import path_util

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

  def GetInstallPath(self) -> str:
    raise NotImplementedError()

  def GetBinaryName(self) -> str:
    raise NotImplementedError()

  def GetSetupDownloadUrl(self, tag) -> str:
    raise NotImplementedError()

  def GetZipDownloadUrl(self, tag) -> str:
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

  def GetInstallPath(self) -> str:
    if sys.platform == 'win32':
      return os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                          'BraveSoftware', 'Brave-Browser-' + self._channel,
                          'Application')

    raise NotImplementedError()

  def GetBinaryName(self) -> str:
    if sys.platform == 'win32':
      return 'brave.exe'

    raise NotImplementedError()

  def GetSetupDownloadUrl(self, tag) -> str:
    return ('https://github.com/brave/brave-browser/releases/download/' +
            f'{tag}/BraveBrowserStandaloneSilentNightlySetup.exe')

  def GetZipDownloadUrl(self, tag) -> str:
    if sys.platform == 'win32':
      platform = 'win32-x64'
      return ('https://github.com/brave/brave-browser/releases/' +
              f'download/{tag}/brave-{tag}-{platform}.zip')
    raise NotImplementedError()


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

  def GetInstallPath(self) -> str:
    if sys.platform == 'win32':
      return os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                          'Google', 'Chrome ' + self._channel,
                          'Application')

    raise NotImplementedError()

  def GetBinaryName(self) -> str:
    if sys.platform == 'win32':
      return 'chrome.exe'

    raise NotImplementedError()

  def GetSetupDownloadUrl(self, tag) -> str:
    return _GetNearestChromiumUrl(tag)

  def GetZipDownloadUrl(self, tag) -> str:
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
