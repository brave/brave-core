# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import re
import logging
import time

from typing import List, Optional

from components.perf_test_utils import GetProcessOutput
import components.path_util as path_util


def RunAsRoot(cmd: str) -> bool:
  result, _ = GetProcessOutput(
      [path_util.GetAdbPath(), 'shell', 'su', '-c', '\'' + cmd + '\''])
  return result


def SetupAndroidDevice() -> None:
  '''Pushes and run setup_android_device.sh script on the device.'''
  tmp_file = '/data/local/tmp/setup_android_device.sh'
  GetProcessOutput([
      path_util.GetAdbPath(),
      'push',
      os.path.join(path_util.GetBravePerfDir(), 'setup_android_device.sh'),
      tmp_file,
  ],
                   check=True)
  if not RunAsRoot(f'sh {tmp_file}'):
    raise RuntimeError('Failed to setup the device: setup_android_device.sh')
  RunAsRoot('killall adbd')  # Restart adbd on the device to apply the changes
  time.sleep(2)


def RebootAndroid() -> None:
  logging.debug('Rebooting Android device')
  RunAsRoot('reboot')

  # Polling to check if the device is ready
  attempts_left = 30
  while attempts_left > 0:
    time.sleep(5)
    result, output = GetProcessOutput(
        [path_util.GetAdbPath(), 'shell', 'getprop', 'sys.boot_completed'])
    if result and output.strip() == '1':
      time.sleep(10)  # Extra delay to make sure the device is ready
      logging.debug('Device is ready after reboot')
      return
    attempts_left -= 1

  raise RuntimeError('Device did not become ready after reboot')


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
