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


def GetBraveScriptDir() -> str:
  return os.path.join(GetBraveDir(), 'script')


def GetCatapultDir() -> str:
  return os.path.join(GetSrcDir(), 'third_party', 'catapult')


def GetTelemetryDir() -> str:
  return os.path.join(GetCatapultDir(), 'telemetry')


def GetGoogleAuthDir() -> str:
  return os.path.join(GetCatapultDir(), 'third_party', 'google-auth')


def GetAdbPath() -> str:
  return os.path.join(GetSrcDir(), 'third_party', 'android_sdk', 'public',
                      'platform-tools', 'adb')


def GetPageSetsDataPath(filename: str) -> str:
  return os.path.join(GetBravePerfDir(), 'brave_page_sets', 'data', filename)


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
