# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import contextlib
import os
import sys


def GetSrcDir() -> str:
  return os.path.abspath(
      os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir,
                   os.pardir))


def GetBraveDir() -> str:
  return os.path.join(GetSrcDir(), 'brave')


def GetBravePerfDir() -> str:
  return os.path.join(GetSrcDir(), 'brave', 'tools', 'perf')


def GetChromiumPerfDir() -> str:
  return os.path.join(GetSrcDir(), 'tools', 'perf')


def GetBravePerfProfileDir() -> str:
  return os.path.join(GetBravePerfDir(), 'profiles')


def GetBravePerfConfigDir() -> str:
  return os.path.join(GetBravePerfDir(), 'configs')


def GetBravePerfBucket() -> str:
  return 'brave-telemetry'


def GetDepotToolsDir() -> str:
  return os.path.join(GetBraveDir(), 'vendor', 'depot_tools')


def GetPyJson5Dir() -> str:
  return os.path.join(GetSrcDir(), 'third_party', 'pyjson5', 'src')


def GetGoogleAuthDir() -> str:
  return os.path.join(GetSrcDir(), 'third_party', 'catapult', 'third_party',
                      'google-auth')


def GetVpython3Path() -> str:
  return os.path.join(GetDepotToolsDir(),
                      'vpython3.bat' if sys.platform == 'win32' else 'vpython3')


def GetChromeReleasesJsonPath() -> str:
  return os.path.join(GetBravePerfDir(), 'chrome_releases.json')


@contextlib.contextmanager
def SysPath(path, position=None):
  if position is None:
    sys.path.append(path)
  else:
    sys.path.insert(position, path)
  try:
    yield
  finally:
    if sys.path[-1] == path:
      sys.path.pop()
    else:
      sys.path.remove(path)


def GetBinaryPath(browser_dir) -> str:
  if sys.platform == 'win32':
    return os.path.join(browser_dir, 'brave.exe')

  raise RuntimeError(f'Unsupported platfrom {sys.platform}')
