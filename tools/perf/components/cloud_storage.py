# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
import hashlib
import logging
import os

from enum import Enum
from pathlib import Path
import tempfile
import time
from urllib.parse import urlparse
from urllib.request import urlopen

import components.perf_test_utils as perf_test_utils
import components.path_util as path_util

with path_util.SysPath(path_util.GetDepotToolsDir()):
  from download_from_google_storage import get_sha1

_CLOUD_BUCKET = 'brave-perf-data'
_CLOUD_HTTPS_URL = 'https://perf-data.s3.brave.com'
_CACHE_DIRECTORY_NAME = 'brave-perf-test-cache'

class CloudFolder(str, Enum):
  TEST_PROFILES = 'perf-profiles'
  CATAPULT_PERF_DATA = 'telemetry-perf-data'


def _ClearOldEntries(cache: str):
  if hasattr(_ClearOldEntries, 'called'):
    return
  _ClearOldEntries.called = True

  day = 86400
  limit_time = time.time() - 7 * day
  for f in Path(cache).glob('**/*'):
    if f.is_file() and os.stat(f).st_mtime < limit_time:
      os.unlink(f)


# Download the URL to `target_filename`, don't put in the local cache.
def DownloadFileOnce(url: str, target_filename: str):

  def load_data():
    for _ in range(3):
      try:
        logging.info('Downloading %s to %s', url, target_filename)
        f = urlopen(url)
        return f.read()
      except Exception:
        logging.error('Download attempt failed')
        time.sleep(5)
    raise RuntimeError(f'Can\'t download {url}')

  data = load_data()
  os.makedirs(os.path.dirname(target_filename), exist_ok=True)
  with open(target_filename, 'wb') as output_file:
    output_file.write(data)


# Downloads the URL, puts it to the local cache (under _CACHE_DIRECTORY_NAME)
# and makes a symlink (target_filename => the location in cache).
def DownloadFile(url: str, target_filename: str):
  if os.path.isfile(target_filename):
    os.unlink(target_filename)
  url_hash = hashlib.sha1(url.encode()).hexdigest()

  cached_location = os.path.join(tempfile.gettempdir(), _CACHE_DIRECTORY_NAME)
  _ClearOldEntries(cached_location)

  filepath = os.path.join(cached_location, url_hash)

  if os.path.isfile(filepath):
    os.symlink(filepath, target_filename)
    os.utime(filepath)  # update st_mtime to prevent removing as a stale.
    return

  DownloadFileOnce(url, filepath)
  os.symlink(filepath, target_filename)
  with open(filepath + '.urlinfo', 'w') as f:
    f.write(url + '\n')


def DownloadFileFromCloudStorage(folder: CloudFolder, sha1: str, output: str):
  assert perf_test_utils.IsSha1Hash(sha1)
  url = f'{_CLOUD_HTTPS_URL}/{folder}/{sha1}'
  DownloadFile(url, output)


def UploadFileToCloudStorage(folder: CloudFolder, path: str):
  assert os.path.isfile(path)
  sha1 = get_sha1(path)
  sha1_path = path + '.sha1'
  with open(sha1_path, 'w', encoding='utf-8') as f:
    f.write(sha1 + '\n')

  s3_url = f's3://{_CLOUD_BUCKET}/{folder}/{sha1}'
  success, _ = perf_test_utils.GetProcessOutput(
      ['aws', 's3', 'cp', path, s3_url, '--sse', 'AES256'])
  if not success:
    raise RuntimeError(f'Can\'t upload to {s3_url}')
  return sha1_path
