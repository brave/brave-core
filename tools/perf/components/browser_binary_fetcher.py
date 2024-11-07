# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# pylint: disable=too-few-public-methods

import os
from typing import List, Optional

from components.field_trials import (FieldTrialConfig, FieldTrialsMode,
                                     MaybeInjectSeedToLocalState,
                                     MakeFieldTrials)
import components.path_util as path_util
import components.field_trials as field_trials
from components.android_tools import InstallApk
from components.common_options import CommonOptions
from components.browser_type import BrowserType
from components.perf_config import RunnerConfig
from components.perf_profile import GetProfilePath

with path_util.SysPath(path_util.GetTelemetryDir()):
  from telemetry.internal.backends import \
      android_browser_backend_settings  # pylint: disable=import-error


class BrowserBinary:
  binary_path: Optional[str] = None
  android_package: Optional[str] = None

  profile_dir: Optional[str] = None
  field_trial_config: FieldTrialConfig
  _browser_type: BrowserType

  def __init__(self, browser_type: BrowserType, binary_path: Optional[str],
               android_package: Optional[str], profile_dir: Optional[str],
               field_trial_config: FieldTrialConfig):
    self._browser_type = browser_type
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

  def get_run_benchmark_args(self) -> List[str]:
    args: List[str] = []

    # Specify the source profile to use:
    if self.profile_dir:
      args.append(f'--profile-dir={self.profile_dir}')

    # Specify the browser binary to run:
    if self.telemetry_browser_type() is not None:
      args.append(f'--browser={self.telemetry_browser_type()}')
    elif self.binary_path is not None:
      args.append('--browser=exact')
      args.append(f'--browser-executable={self.binary_path}')
    else:
      raise RuntimeError('Bad binary spec, no browser to run')

    args.extend(self._browser_type.extra_benchmark_args)

    if self.field_trial_config.mode != FieldTrialsMode.TESTING_FIELD_TRIALS:
      # Don't use chromium mechanism to inject field trials
      args.append('--compatibility-mode=no-field-trials')

    return args

  def get_browser_args(self) -> List[str]:
    args: List[str] = []
    if self.field_trial_config.mode != FieldTrialsMode.TESTING_FIELD_TRIALS:
      args.append('--disable-field-trial-config')
      args.append('--accept-empty-variations-seed-signature')
      args.append('--variations-override-country=us')
      if self.field_trial_config.fake_channel:
        args.append('--fake-variations-channel=' +
                    self.field_trial_config.fake_channel)
    args.extend(self._browser_type.extra_browser_args)
    return args

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

  trials = config.field_trials or config.browser_type.GetDefaultFieldTrials()
  field_trial_config = MakeFieldTrials(trials, artifacts_dir, config.version,
                                       common_options.variations_repo_dir)
  MaybeInjectSeedToLocalState(field_trial_config, profile_dir)

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
    return BrowserBinary(config.browser_type, binary_location, None,
                         profile_dir, field_trial_config)

  if package is None:
    assert binary_location is not None
    assert binary_location.endswith('.apk')

  return BrowserBinary(config.browser_type, binary_location, package,
                       profile_dir, field_trial_config)
