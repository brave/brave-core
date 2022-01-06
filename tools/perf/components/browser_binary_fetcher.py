# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
import os
import shutil
import sys
import re
from typing import Tuple, Optional

from urllib.request import urlopen
from io import BytesIO
from zipfile import ZipFile
from distutils.dir_util import copy_tree

from components.browser_type import BrowserType
from components.perf_test_utils import GetProcessOutput
from components import path_util

def DownloadArchiveAndUnpack(output_directory: str, url: str) -> str:
  logging.info('Downloading archive %s', url)
  resp = urlopen(url)
  zipfile = ZipFile(BytesIO(resp.read()))
  zipfile.extractall(output_directory)
  return os.path.join(output_directory,
                      path_util.GetBinaryPath(output_directory))


def DownloadWinInstallerAndExtract(out_dir: str,
                                   url: str,
                                   browser_type: BrowserType):
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  installer_filename = os.path.join(out_dir, os.pardir, 'temp_installer.exe')
  expected_install_path = browser_type.GetInstallPath()
  binary_name = browser_type.GetBinaryName()
  logging.info('Downloading %s', url)
  f = urlopen(url)
  data = f.read()
  with open(installer_filename, 'wb') as output_file:
    output_file.write(data)
  GetProcessOutput([installer_filename, '--chrome-sxs',
                    '--do-not-launch-chrome'], None, True)

  GetProcessOutput(['taskkill.exe', '/f', '/im', binary_name], None, True)

  if not os.path.exists(expected_install_path):
    raise RuntimeError(f'No files found in {expected_install_path}')

  full_version = None
  logging.info('Copy files to %s', out_dir)
  copy_tree(expected_install_path, out_dir)
  for file in os.listdir(expected_install_path):
    if re.match(r'\d+\.\d+\.\d+.\d+', file):
      assert (full_version is None)
      full_version = file
  assert (full_version is not None)
  logging.info('Detected version %s', full_version)
  setup_filename = os.path.join(expected_install_path, full_version,
                                'Installer', 'setup.exe')
  logging.info('Run uninstall')

  GetProcessOutput([setup_filename, '--uninstall',
                    '--force-uninstall', '--chrome-sxs'])
  shutil.rmtree(expected_install_path, True)

  return os.path.join(out_dir, binary_name)


def ParseTarget(target: str) -> Tuple[Optional[str], str]:
  m = re.match(r'^(v\d+\.\d+\.\d+)(?::(.+)|$)', target)
  if not m:
    return None, target
  logging.debug('Parsed tag: %s, location : %s', m.group(1), m.group(2))
  return m.group(1), m.group(2)


def PrepareBinaryByTag(out_dir: str,
                       tag: str,
                       browser_type: BrowserType) -> str:
  m = re.match(r'^v(\d+)\.(\d+)\.\d+$', tag)
  if not m:
    raise RuntimeError(f'Failed to parse tag "{tag}"')

  # win nightly < v1.35 has a broken .zip archive
  if sys.platform == 'win32' and int(m.group(1)) == 1 and int(m.group(2)) < 35:
    return DownloadWinInstallerAndExtract(out_dir,
                                          browser_type.GetSetupDownloadUrl(
                                              tag),
                                          browser_type)
  return DownloadArchiveAndUnpack(out_dir, browser_type.GetZipDownloadUrl(tag))


def PrepareBinary(out_dir: str,
                  tag: str,
                  location: Optional[str],
                  browser_type: BrowserType) -> str:
  if location:  # local binary
    if os.path.exists(location):
      return location
    raise RuntimeError(f'{location} doesn\'t exist')
  return PrepareBinaryByTag(out_dir, tag, browser_type)
