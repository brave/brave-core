# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import glob
import os
import sys
from typing import List, Tuple

# A standard workflow to generate pgo profiles:
# 1. Make a release build with chrome_pgo_phase=1
# 2. Run the benchmarks. .profraw files will be saved in the artifacts.
# 3. Download chromium pgo profiles
# 4. Merge the profraw files with chromium profiles
# 5. Upload it to s3
# 6. Update the

from components.perf_test_utils import GetProcessOutput
import components.path_util as path_util
import components.cloud_storage as cloud_storage

with path_util.SysPath(path_util.GetDepotToolsDir()):
  import download_from_google_storage

SUPPORTED_TARGETS = ['win64']
CHROMIUM_GS_PATH = 'chromium-optimization-profiles/pgo_profiles'


def GetUpdatePy() -> str:
  return f'{path_util.GetSrcDir()}/tools/clang/scripts/update.py'


def GetLLVMDir() -> str:
  return f'{path_util.GetSrcDir()}/third_party/llvm-build/Release+Asserts'


def GetProfdata() -> str:
  EXE_EXT = '.exe' if sys.platform == 'win32' else ''
  return f'{GetLLVMDir()}/bin/llvm-profdata{EXE_EXT}'


def EnsureProfdata() -> None:
  if not os.path.exists(GetProfdata()):
    GetProcessOutput(
        [sys.executable,
         GetUpdatePy(), '--package=coverage_tools'], check=True)
  assert os.path.exists(GetProfdata()), f'can\'t downloaded {GetProfdata()} '


def RunProfdataMerge(output_path: str, input_files: List[Tuple[int,
                                                               str]]) -> None:
  EnsureProfdata()

  cmd = [GetProfdata(), 'merge', '-o', output_path] + [
      f'--weighted-input={weight},{input_file}'
      for weight, input_file in input_files
  ]
  _, output = GetProcessOutput(cmd, check=True)
  if 'invalid profile created' in output:
    raise RuntimeError(f'Failed to merge profdata: {output}')


def _read_profile_name(target):
  """Read profile name given a target.

  Args:
    target(str): The target name, such as win32, mac.

  Returns:
    Name of the profile to update and use, such as:
    chrome-win32-master-67ad3c89d2017131cc9ce664a1580315517550d1.profdata.
  """
  _PGO_DIR = os.path.join(path_util.GetSrcDir(), 'chrome', 'build')
  state_file = os.path.join(_PGO_DIR, '%s.pgo.txt' % target)
  with open(state_file, 'r') as f:
    profile_name = f.read().strip()

  return profile_name


def DownloadChromiumPgoProfiles(target: str, output_path: str) -> None:
  """
  Download the chromium pgo profiles for the given target and save it to the output path.
  Keep sync with tools/update_pgo_profiles.py
  """
  assert target in SUPPORTED_TARGETS, f'unsupported target: {target}'
  gsutil = download_from_google_storage.Gsutil(
      download_from_google_storage.GSUTIL_DEFAULT_PATH)
  gs_path = f'gs://{CHROMIUM_GS_PATH}/{_read_profile_name(target)}'
  code = gsutil.call('cp', gs_path, output_path)
  if code != 0:
    raise RuntimeError('gsutil failed to download "%s"' % gs_path)


def ProcessAndUploadPgoProfile(platform: str, working_directory: str) -> None:
  # TODO: clarify the directory structure
  glob_pattern = os.path.join(working_directory, '**', '*.profraw')
  profraw_files: List[Tuple[int, str]] = []
  for file in glob.glob(glob_pattern, recursive=True):
    print(f'Processing {file}')
    profraw_files.append((10, file))

  if len(profraw_files) == 0:
    # If there is no profraw files, check that:
    # 1. CHROME_PGO_PROFILING=1 is set
    # 2. The binary has been built with chrome_pgo_phase=1
    raise RuntimeError('No profraw files found')

  chromium_pgo_path = os.path.join(working_directory,
                                   'chromium_pgo_profiles.profdata')
  DownloadChromiumPgoProfiles(platform, chromium_pgo_path)
  profraw_files.append((1, chromium_pgo_path))

  output_file = path_util.GetBravePgoProfilesPath(platform)
  RunProfdataMerge(output_file, profraw_files)

  # sha1_path = cloud_storage.UploadFileToCloudStorage(
  #   cloud_storage.CloudFolder.PGO_PROFILES,
  #   output_file)

  # TODO: make a commit
