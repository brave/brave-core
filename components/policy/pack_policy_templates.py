#!/usr/bin/env python3
# Copyright 2022 The Brave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os

from os.path import join, exists, relpath
from tempfile import TemporaryDirectory
from zipfile import ZipFile

def main():
  chrome_policy_zip, dest_zip = _get_args()
  _pack_policy_templates(chrome_policy_zip, dest_zip)

def _get_args():
  parser = argparse.ArgumentParser()
  parser.add_argument('chrome_policy_zip',
                      help="Path to Chrome's policy_templates.zip")
  parser.add_argument('dest_zip',
                      help="Path to the Zip file to be created")
  args = parser.parse_args()
  return args.chrome_policy_zip, args.dest_zip

def _pack_policy_templates(chrome_policy_zip, dest_zip):
  with TemporaryDirectory() as tmp_dir:
    with ZipFile(chrome_policy_zip) as src_zip:
      src_zip.extract('VERSION', tmp_dir)
      namelist = src_zip.namelist()
      for dir_ in ('windows/adm/', 'windows/admx/', 'windows/examples/'):
        src_zip.extractall(tmp_dir, (n for n in namelist if n.startswith(dir_)))

    # Some sanity checks:
    assert exists(join(tmp_dir, 'windows/adm/en-US/chrome.adm'))
    assert exists(join(tmp_dir, 'windows/admx/chrome.admx'))
    assert exists(join(tmp_dir, 'windows/admx/en-US/chrome.adml'))

    with ZipFile(dest_zip, 'w') as dest_zipfile:
       for dirpath, dirnames, filenames in os.walk(tmp_dir):
         for filename in filenames:
           filepath = join(dirpath, filename)
           arcname = relpath(filepath, tmp_dir).replace('chrome', 'brave')
           dest_zipfile.write(filepath, arcname=arcname)

if '__main__' == __name__:
  main()
