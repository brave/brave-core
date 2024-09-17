#!/usr/bin/env vpython3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import logging
import os

import components.wpr_utils as wpr_utils
import components.cloud_storage as cloud_storage


def main():
  logging.basicConfig(level=logging.INFO, format='%(asctime)s: %(message)s')

  parser = argparse.ArgumentParser()
  subparsers = parser.add_subparsers(dest='subparser_name')
  wpr_parser = subparsers.add_parser('wpr')
  wpr_parser.add_argument('cmd', type=str, choices=['cleanup', 'ls'])
  s3_parser = subparsers.add_parser('s3')
  s3_parser.add_argument('cmd', type=str, choices=['upload', 'update-sha1'])

  parser.add_argument('file', type=str)
  args = parser.parse_args()
  if args.subparser_name == 'wpr':
    if args.cmd == 'cleanup':
      wpr_utils.cleanup_archive(args.file, True)

    if args.cmd == 'ls':
      print(wpr_utils.run_httparchive(['ls', os.path.abspath(args.file)]))

  if args.subparser_name == 's3':
    if args.cmd == 'upload':
      cloud_storage.UploadFileToCloudStorage(
          cloud_storage.CloudFolder.CATAPULT_PERF_DATA, args.file)

    if args.cmd == 'update-sha1':
      cloud_storage.UpdateSha1(args.file)


main()
