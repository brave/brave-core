# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

# pylint: disable=too-few-public-methods

import os
import re
from typing import Optional

import components.path_util as path_util
from components.browser_type import BrowserType
from components.common_options import CommonOptions
from components.perf_config import RunnerConfig
from components.perf_profile import GetProfilePath
from components.perf_test_utils import GetProcessOutput
from components.version import BraveVersion

with path_util.SysPath(path_util.GetTelemetryDir()):
  from telemetry.internal.backends import \
      android_browser_backend_settings  # pylint: disable=import-error


class BrowserBinary:
  binary_path: Optional[str] = None
  android_package: Optional[str] = None
  telemetry_browser_type: Optional[str] = None

  profile_dir: Optional[str] = None
  field_trial_config: Optional[str] = None

  def __init__(self, binary_path: Optional[str], android_package: Optional[str],
               profile_dir: Optional[str], field_trial_config: Optional[str]):
    self.binary_path = binary_path
    self.android_package = android_package
    self.profile_dir = profile_dir
    self.field_trial_config = field_trial_config
    if android_package is not None:
      for b in android_browser_backend_settings.ANDROID_BACKEND_SETTINGS:
        if b.package == android_package:
          self.telemetry_browser_type = b.browser_type
      if self.telemetry_browser_type is None:
        raise RuntimeError('No matching browser-type found ' + android_package)
    else:
      assert binary_path is not None

  def __str__(self) -> str:
    if self.binary_path:
      return self.binary_path
    if self.android_package:
      return 'package:' + self.android_package
    return '<empty>'


def _GetPackageName(apk_path: str) -> str:
  aapt2 = os.path.join(path_util.GetSrcDir(), 'third_party',
                       'android_build_tools', 'aapt2', 'aapt2')
  assert apk_path.endswith('.apk')
  _, aapt2_info = GetProcessOutput([aapt2, 'dump', 'badging', apk_path],
                                   check=True)
  package_match = re.search(r'package: name=\'((?:\w|\.)+)\'', aapt2_info)
  assert package_match is not None
  return package_match.group(1)


def _GetPackageVersion(package: str) -> str:
  _, dump_info = GetProcessOutput(
      [path_util.GetAdbPath(), 'shell', 'dumpsys', 'package', package],
      check=True)
  version_match = re.search(r'versionName=((?:\w|\.)+)', dump_info)
  assert version_match is not None
  return version_match.group(1)


def _InstallApk(apk_path: str, browser_type: BrowserType,
                version: Optional[BraveVersion]) -> str:
  assert apk_path.endswith('.apk')
  package = _GetPackageName(apk_path)

  adb = path_util.GetAdbPath()

  GetProcessOutput([adb, 'uninstall', package])
  GetProcessOutput([adb, 'install', apk_path], check=True)

  # grant the permissions to prevent showing popup
  GetProcessOutput([
      adb, 'shell', 'pm', 'grant', package,
      'android.permission.POST_NOTIFICATIONS'
  ],
                   check=True)

  # stop all the other browsers to avoid an interference.
  GetProcessOutput([
      adb, 'shell',
      ('ps -o NAME -A' +
       '| grep -e com.brave -e com.chrome -e com.chromium -e android.chrome' +
       '| xargs -r -n 1 am force-stop')
  ],
                   check=True)

  if browser_type.win_name.startswith('brave') and version is not None:
    installed_version = 'v' + _GetPackageVersion(package)
    if installed_version != version.last_tag:
      raise RuntimeError('Version mismatch: expected ' +
                         f'{version.to_string()}, installed {version}')

  return package


def _PostInstallAndroidSetup(package: str):
  GetProcessOutput([
      path_util.GetAdbPath(), 'shell', 'pm', 'grant', package,
      'android.permission.POST_NOTIFICATIONS'
  ],
                   check=True)


def PrepareBinary(binary_dir: str, artifacts_dir: str, config: RunnerConfig,
                  common_options: CommonOptions) -> BrowserBinary:

  profile_dir = None
  if config.profile != 'clean':
    profile_dir = GetProfilePath(config.profile,
                                 common_options.working_directory)

  field_trial_config = config.browser_type.MakeFieldTrials(
      config.version, artifacts_dir, common_options)

  binary_location = None
  package = None
  url: Optional[str] = None
  is_android = common_options.target_os == 'android'
  if config.location:  # explicit binary/archive location
    if os.path.exists(config.location):
      binary_location = config.location
    elif config.location.startswith('https:'):
      url = config.location
    elif config.location.startswith('package:') and is_android:
      package = config.location[len('package:'):]
    else:
      raise RuntimeError(f'Bad explicit location {config.location}')

  if binary_location is None and package is None:
    assert config.version is not None
    binary_location = config.browser_type.DownloadBrowserBinary(
        url, config.version, binary_dir, common_options)

  if not is_android:
    assert binary_location
    return BrowserBinary(binary_location, None, profile_dir, field_trial_config)

  if package is None:
    assert binary_location
    assert binary_location.endswith('.apk')
    package = _InstallApk(binary_location, config.browser_type, config.version)

  _PostInstallAndroidSetup(package)
  return BrowserBinary(binary_location, package, profile_dir,
                       field_trial_config)
