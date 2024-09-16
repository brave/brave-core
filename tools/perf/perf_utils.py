#!/usr/bin/env vpython3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import logging

import components.wpr_utils as wpr_utils
import components.cloud_storage as cloud_storage


logging.basicConfig(level=logging.INFO, format='%(asctime)s: %(message)s')

parser = argparse.ArgumentParser()
parser.add_argument(
      'mode',
      type=str,
      choices=['wpr-cleanup', 'wpr-ls', 's3-upload', 'wpr-upload'])
parser.add_argument(
      'file',
      type=str)
args = parser.parse_args()
if args.mode == 'wpr-cleanup':
  wpr_utils.cleanup_archive(args.file, True)

if args.mode == 'wpr-ls':
  print(wpr_utils.run_httparchive(['ls', args.file]))

if args.mode == 'wpr-upload':
  cloud_storage.UploadFileToCloudStorage(cloud_storage.CloudFolder.CATAPULT_PERF_DATA, args.file)
