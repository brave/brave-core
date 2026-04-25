#!/usr/bin/env vpython3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"A set of service tools for perf tests. Use --help to get details."
import argparse
import logging
import os

import components.wpr_utils as wpr_utils
import components.cloud_storage as cloud_storage


def main():
  logging.basicConfig(level=logging.INFO, format='%(asctime)s: %(message)s')

  parser = argparse.ArgumentParser(
      description='A set of supporting tools for the perf tests')
  subparsers = parser.add_subparsers(dest='subparser_name',
                                     help='A subcommand to run')
  wpr_cleanup_parser = subparsers.add_parser(
      'wpr-cleanup',
      help='Rewrite the WPR file to drop the unwanted and duplicated requests.')
  wpr_cleanup_parser.add_argument(
      '--preseve-service-hosts',
      action='store_true',
      default=False,
      help='Don\'t remove requests to the service hosts (i.e. rewards.brave.com)'
  )
  subparsers.add_parser('wpr-ls', help='List the content of WPR file.')
  subparsers.add_parser(
      's3-upload',
      help=
      'Upload the file to cloud storage. <file>.sha1 will be updated automatically'
  )
  subparsers.add_parser(
      's3-update-sha1',
      help='Calculate the hash and update the corresponding <file>.sha1 file')

  parser.add_argument('file', type=str, help='the input file to process')

  args = parser.parse_args()
  if args.subparser_name == 'wpr-cleanup':
    wpr_utils.cleanup_archive(args.file, not args.preseve_service_hosts)

  if args.subparser_name == 'wpr-ls':
    print(wpr_utils.run_httparchive(['ls', os.path.abspath(args.file)]))

  if args.subparser_name == 's3-upload':
    cloud_storage.UploadFileToCloudStorage(
        cloud_storage.CloudFolder.CATAPULT_PERF_DATA, args.file)

  if args.subparser_name == 's3-update-sha1':
    cloud_storage.UpdateSha1(args.file)


main()
