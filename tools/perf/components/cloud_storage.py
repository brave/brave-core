# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
import os

from enum import Enum

import components.perf_test_utils as perf_test_utils

from download_from_google_storage import get_sha1

_CLOUD_BUCKET = 'brave-perf-data'
_CLOUD_HTTPS_URL = 'https://perf-data.s3.brave.com'


class CloudFolder(str, Enum):
  TEST_PROFILES = 'perf-profiles'
  CATAPULT_PERF_DATA = 'telemetry-perf-data'


def DownloadFileFromCloudStorage(folder: CloudFolder, sha1: str, output: str):
  assert perf_test_utils.IsSha1Hash(sha1)
  url = f'{_CLOUD_HTTPS_URL}/{folder}/{sha1}'
  perf_test_utils.DownloadFile(url, output)


def UploadFileToCloudStorage(folder: CloudFolder, path: str):
  assert os.path.isfile(path)
  sha1 = get_sha1(path)
  sha1_path = path + '.sha1'
  with open(sha1_path, 'w', encoding='utf-8') as f:
    f.write(sha1 + '\n')

  s3_url = f's3://{_CLOUD_BUCKET}/{folder}/{sha1}'
  success, _ = perf_test_utils.GetProcessOutput(
      ['aws', 's3', 'cp', path, s3_url])
  if not success:
    raise RuntimeError(f'Can\'t upload to {s3_url}')
  return sha1_path
