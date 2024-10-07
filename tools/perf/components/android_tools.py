# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import re
import logging

from typing import Optional

from components.perf_test_utils import GetProcessOutput
import components.path_util as path_util


def GetPackageVersion(package: str) -> str:
  _, dump_info = GetProcessOutput(
      [path_util.GetAdbPath(), 'shell', 'dumpsys', 'package', package],
      check=True,
      output_to_debug=False)
  version_match = re.search(r'versionName=((?:\w|\.)+)', dump_info)
  assert version_match is not None
  return version_match.group(1)


def GetPackageName(apk_path: str) -> str:
  aapt2 = os.path.join(path_util.GetSrcDir(), 'third_party',
                       'android_build_tools', 'aapt2', 'cipd', 'aapt2')
  assert apk_path.endswith('.apk')
  _, aapt2_info = GetProcessOutput([aapt2, 'dump', 'badging', apk_path],
                                   check=True)
  package_match = re.search(r'package: name=\'((?:\w|\.)+)\'', aapt2_info)
  assert package_match is not None
  return package_match.group(1)


def InstallApk(apk_path: str, expected_version: Optional[str]) -> str:
  assert apk_path.endswith('.apk')
  package = GetPackageName(apk_path)

  adb = path_util.GetAdbPath()

  GetProcessOutput([adb, 'uninstall', package])

  logging.debug('Installing: %s', apk_path)
  GetProcessOutput([adb, 'install', apk_path], check=True)

  installed_version = GetPackageVersion(package)
  logging.debug('Installed version: %s', installed_version)

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

  if expected_version is not None and installed_version != expected_version:
    raise RuntimeError('Version mismatch: expected ' +
                       f'{expected_version}, installed {installed_version}')

  return package
