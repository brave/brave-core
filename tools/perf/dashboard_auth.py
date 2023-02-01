#!/usr/bin/env python3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
"""A tool to perform authorization to https://brave-perf-dashboard.appspot.com/"""

import sys
import json
import io
import argparse

from google.oauth2 import service_account
from google.auth.transport.requests import Request

SCOPES = ['openid', 'https://www.googleapis.com/auth/userinfo.email']


def GetDashboardToken(file: io.TextIOWrapper) -> str:
  service_account_info = json.load(file)
  credentials = service_account.Credentials.from_service_account_info(
      service_account_info, scopes=SCOPES)
  if credentials is None:
    raise RuntimeError('Error reading authentication token')
  credentials.refresh(Request())
  if credentials.token is None:
    raise RuntimeError('Can\'t get the token')
  return credentials.token


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('service_account_json_file',
                      type=argparse.FileType('r'),
                      nargs='?',
                      default=sys.stdin,
                      help='json file with a service account private key')
  args = parser.parse_args()

  token = GetDashboardToken(args.service_account_json_file)
  print(token)


if __name__ == '__main__':
  sys.exit(main())
