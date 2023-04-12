# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
import os
import re
from typing import Tuple, Optional

import components.path_util as path_util
from components.browser_type import BrowserType, BraveVersion
from components.perf_test_utils import GetProcessOutput

with path_util.SysPath(path_util.GetTelemetryDir()):
  from telemetry.internal.backends import android_browser_backend_settings  # pylint: disable=import-error


class BrowserBinary:
  binary_path: Optional[str] = None
  android_package: Optional[str] = None
  telemetry_browser_type: Optional[str] = None

  @classmethod
  def from_binary(cls, binary_path: str) -> 'BrowserBinary':
    binary = BrowserBinary()
    binary.binary_path = binary_path
    return binary

  @classmethod
  def from_android_package(cls, android_package: str) -> 'BrowserBinary':
    binary = BrowserBinary()
    binary.android_package = android_package
    for b in android_browser_backend_settings.ANDROID_BACKEND_SETTINGS:
      if b.package == android_package:
        binary.telemetry_browser_type = b.browser_type
    if binary.telemetry_browser_type is None:
      raise RuntimeError('No matching matching browser-type found')

    return binary

  def __str__(self) -> str:
    if self.binary_path:
      return self.binary_path
    if self.android_package:
      return 'package:' + self.android_package
    return '<empty>'


def ParseTarget(target: str) -> Tuple[Optional[BraveVersion], str]:
  m = re.match(r'^(v\d+\.\d+\.\d+)(?::(.+)|$)', target)
  if not m:
    return None, target
  tag = BraveVersion(m.group(1))
  location = m.group(2)
  logging.debug('Parsed tag: %s, location : %s', tag, location)
  return tag, location


def _GetPackageName(apk_path: str) -> str:
  aapt2 = os.path.join(path_util.GetSrcDir(), 'third_party',
                       'android_build_tools', 'aapt2', 'aapt2')
  assert (apk_path.endswith('.apk'))
  _, aapt2_info = GetProcessOutput([aapt2, 'dump', 'badging', apk_path],
                                   check=True)
  packege_match = re.search(r'package: name=\'((?:\w|\.)+)\'', aapt2_info)
  assert (packege_match is not None)
  return packege_match.group(1)


def _GetPackageVersion(package: str) -> str:
  _, dump_info = GetProcessOutput(
      [path_util.GetAdbPath(), 'shell', 'dumpsys', 'package', package],
      check=True)
  version_match = re.search(r'versionName=((?:\w|\.)+)', dump_info)
  assert (version_match is not None)
  return version_match.group(1)


def _InstallApk(apk_path: str, browser_type: BrowserType,
                tag: Optional[BraveVersion]) -> str:
  assert (apk_path.endswith('.apk'))
  package = _GetPackageName(apk_path)

  adb = path_util.GetAdbPath()

  GetProcessOutput([adb, 'uninstall', package])
  GetProcessOutput([adb, 'install', apk_path], check=True)

  GetProcessOutput([
      adb, 'shell', 'pm', 'grant', package,
      'android.permission.POST_NOTIFICATIONS'
  ],
                   check=True)

  if browser_type.GetName().startswith('brave'):
    version = 'v' + _GetPackageVersion(package)
    if version != tag.__str__():
      raise RuntimeError(f'Version mismatch: tag {tag}, installed {version}')

  return package


def _PostInstallAndroidSetup(package: str):
  GetProcessOutput([
      path_util.GetAdbPath(), 'shell', 'pm', 'grant', package,
      'android.permission.POST_NOTIFICATIONS'
  ],
                   check=True)


def PrepareBinary(out_dir: str, tag: Optional[BraveVersion],
                  location: Optional[str], browser_type: BrowserType,
                  target_os: str) -> BrowserBinary:
  binary_location = None
  package = None
  if location:  # local binary
    if os.path.exists(location):
      binary_location = location
    elif location.startswith('package:') and target_os == 'android':
      package = location[len('package:'):]
    else:
      raise RuntimeError(f'{location} doesn\'t exist')
  else:
    assert (tag is not None)
    binary_location = browser_type.DownloadBrowserBinary(
        tag, out_dir, target_os)

  if target_os != 'android':
    assert (binary_location)
    return BrowserBinary.from_binary(binary_location)

  if package is None:
    assert (binary_location)
    assert (binary_location.endswith('.apk'))
    package = _InstallApk(binary_location, browser_type, tag)

  _PostInstallAndroidSetup(package)
  return BrowserBinary.from_android_package(package)
