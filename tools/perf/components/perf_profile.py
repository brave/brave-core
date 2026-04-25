# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import logging
import os
import uuid
import shutil

import components.path_util as path_util

from components.cloud_storage import CloudFolder, DownloadFileFromCloudStorage
from components.version import BraveVersion
from components.git_tools import GetFileAtRevision
from components.perf_test_utils import IsSha1Hash


with path_util.SysPath(path_util.GetBraveScriptDir(), 0):
  from lib.util import extract_zip


def _GetProfileHash(profile: str, version: BraveVersion) -> str:
  if IsSha1Hash(profile):  # the explicit profile hash
    return profile
  sha1_filepath = os.path.join(path_util.GetBravePerfProfileDir(),
                               f'{profile}.zip.sha1')
  sha1_fallback_filepath = sha1_filepath + '.fallback'

  if not os.path.isfile(sha1_filepath) and not os.path.isfile(
      sha1_fallback_filepath):
    raise RuntimeError(
        f'Unknown profile {profile}, file {sha1_filepath}[.fallback] not found')

  sha1 = GetFileAtRevision(sha1_filepath, version.git_revision)
  if sha1 is None:
    logging.info('Using the fallback profile %s', sha1_fallback_filepath)
    if not os.path.isfile(sha1_fallback_filepath):
      raise RuntimeError(
          f'Can\'t find fallback profile {sha1_fallback_filepath}')

    with open(sha1_fallback_filepath, 'r', encoding='utf8') as sha1_file:
      sha1 = sha1_file.read().rstrip()

  if sha1 is None:
    raise RuntimeError(f'Bad sha1 for profile {profile}')
  sha1 = sha1.rstrip()
  logging.debug('Use sha1 hash %s for profile %s', sha1, profile)
  return sha1


def GetProfilePath(profile: str, work_directory: str,
                   version: BraveVersion) -> str:
  assert profile != 'clean'

  profile_dir = None
  if os.path.isdir(profile):  # local profile
    profile_dir = os.path.join(work_directory, 'profiles',
                               uuid.uuid4().hex.upper()[0:6])
    logging.debug('Copy %s to %s ', profile, profile_dir)
    shutil.copytree(profile, profile_dir)
  else:
    sha1 = _GetProfileHash(profile, version)
    zip_path = os.path.join(path_util.GetBravePerfProfileDir(),
                            f'{profile}_{sha1}.zip')

    if not os.path.isfile(zip_path):
      DownloadFileFromCloudStorage(CloudFolder.TEST_PROFILES, sha1, zip_path)

    profile_dir = os.path.join(work_directory, 'profiles', sha1)

    if not os.path.isdir(profile_dir):
      os.makedirs(profile_dir)
      logging.info('Create temp profile dir %s for profile %s', profile_dir,
                   profile)

      extract_zip(zip_path, profile_dir)

  logging.info('Use temp profile dir %s for profile %s', profile_dir, profile)
  return profile_dir
