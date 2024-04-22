# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# pylint: disable=too-few-public-methods

import os
from typing import Optional

import components.path_util as path_util

from components.android_tools import InstallApk
from components.common_options import CommonOptions
from components.browser_type import FieldTrialConfig
from components.perf_config import RunnerConfig
from components.perf_profile import GetProfilePath

with path_util.SysPath(path_util.GetTelemetryDir()):
  from telemetry.internal.backends import \
      android_browser_backend_settings  # pylint: disable=import-error


class BrowserBinary:
  binary_path: Optional[str] = None
  android_package: Optional[str] = None

  profile_dir: Optional[str] = None
  field_trial_config: Optional[FieldTrialConfig] = None

  def __init__(self, binary_path: Optional[str], android_package: Optional[str],
               profile_dir: Optional[str],
               field_trial_config: Optional[FieldTrialConfig]):
    self.binary_path = binary_path
    self.android_package = android_package
    self.profile_dir = profile_dir
    self.field_trial_config = field_trial_config
    if android_package is not None:
      if self.telemetry_browser_type() is None:
        raise RuntimeError('No matching browser-type found ' + android_package)
    else:
      assert binary_path is not None

  def telemetry_browser_type(self) -> Optional[str]:
    if self.android_package is None:
      return None
    for b in android_browser_backend_settings.ANDROID_BACKEND_SETTINGS:
      if b.package == self.android_package:
        return b.browser_type
    return None

  def install_apk(self, expected_version: Optional[str]) -> None:
    assert self.binary_path is not None
    self.android_package = InstallApk(self.binary_path, expected_version)
    if self.telemetry_browser_type() is None:
      raise RuntimeError('No matching browser-type found ' +
                         self.android_package)


  def __str__(self) -> str:
    if self.binary_path:
      return self.binary_path
    if self.android_package:
      return 'package:' + self.android_package
    return '<empty>'


def PrepareBinary(binary_dir: str, artifacts_dir: str, config: RunnerConfig,
                  common_options: CommonOptions) -> BrowserBinary:

  profile_dir = None
  if config.profile != 'clean':
    if config.version is None:
      raise RuntimeError(
          f'Using non-empty profile {config.profile} requires a version')
    profile_dir = GetProfilePath(config.profile,
                                 common_options.working_directory,
                                 config.version)

  field_trial_config = config.browser_type.MakeFieldTrials(
      config.version, artifacts_dir, common_options)

  binary_location = None
  package = None
  url: Optional[str] = None
  if config.location:  # explicit binary/archive location
    if os.path.exists(config.location):
      binary_location = config.location
    elif config.location.startswith('https:'):
      url = config.location
    elif config.location.startswith('package:') and common_options.is_android:
      package = config.location[len('package:'):]
    else:
      raise RuntimeError(f'Bad explicit location {config.location}')

  if binary_location is None and package is None:
    assert config.version is not None
    binary_location = config.browser_type.DownloadBrowserBinary(
        url, config.version, binary_dir, common_options)

  if not common_options.is_android:
    assert binary_location
    return BrowserBinary(binary_location, None, profile_dir, field_trial_config)

  if package is None:
    assert binary_location is not None
    assert binary_location.endswith('.apk')

  return BrowserBinary(binary_location, package, profile_dir,
                       field_trial_config)
