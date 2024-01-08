# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import logging
import os
import uuid
import shutil

import components.path_util as path_util

from components.perf_test_utils import GetFileAtRevision
from components.version import BraveVersion

with path_util.SysPath(path_util.GetBraveScriptDir(), 0):
  from lib.util import extract_zip

with path_util.SysPath(path_util.GetDepotToolsDir()):
  import download_from_google_storage  # pylint: disable=import-error


def DownloadFromGoogleStorage(sha1: str, output_path: str) -> None:
  """Download a file from brave perf bucket.

  To upload a file call:
  upload_to_google_storage.py some_file.zip -b brave-telemetry
  """
  gsutil = download_from_google_storage.Gsutil(
      download_from_google_storage.GSUTIL_DEFAULT_PATH)
  gs_path = 'gs://' + path_util.GetBravePerfBucket() + '/' + sha1
  logging.info('Download profile from %s to %s', gs_path, output_path)
  exit_code = gsutil.call('cp', gs_path, output_path)
  if exit_code:
    raise RuntimeError(f'Failed to download: {gs_path}')


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
    zip_path_sha1 = os.path.join(path_util.GetBravePerfProfileDir(),
                                 f'{profile}.zip.sha1')
    zip_path_sha1_fallback = zip_path_sha1 + '.fallback'

    if not os.path.isfile(zip_path_sha1) and not os.path.isfile(
        zip_path_sha1_fallback):
      raise RuntimeError(
          f'Unknown profile, file {zip_path_sha1}[.fallback] not found')

    sha1 = GetFileAtRevision(zip_path_sha1, version.git_hash)
    if sha1 is None:
      zip_path_sha1 = zip_path_sha1_fallback
      logging.info('Using the fallback profile %s', zip_path_sha1)
      if not os.path.isfile(zip_path_sha1):
        raise RuntimeError(f'No fallback profile for {zip_path_sha1} not found')

      with open(zip_path_sha1, 'r', encoding='utf8') as sha1_file:
        sha1 = sha1_file.read().rstrip()

    assert sha1 is not None
    sha1 = sha1.rstrip()
    logging.debug('Use gs hash %s for profile %s', sha1, profile)

    zip_path = os.path.join(path_util.GetBravePerfProfileDir(),
                            f'{profile}_{sha1}.zip')
    if not sha1:
      raise RuntimeError(f'Bad sha1 in {zip_path_sha1}')

    if not os.path.isfile(zip_path):
      DownloadFromGoogleStorage(sha1, zip_path)

    profile_dir = os.path.join(work_directory, 'profiles', sha1)

    if not os.path.isdir(profile_dir):
      os.makedirs(profile_dir)
      logging.info('Create temp profile dir %s for profile %s', profile_dir,
                   profile)

      extract_zip(zip_path, profile_dir)

  logging.info('Use temp profile dir %s for profile %s', profile_dir, profile)
  return profile_dir
