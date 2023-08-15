#!/usr/bin/env python3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A tool to perform authorization to brave-perf-dashboard.appspot.com"""

import sys
import json
import io
import os
import argparse
import logging

from typing import Optional

# pytype: disable=import-error
# pylint: disable=import-error
from google.oauth2 import service_account
from google.auth.transport.requests import Request

SERVICE_ACCOUNT_FILE_ENV_NAME = 'PERF_DASHBOARD_SERVICE_ACCOUNT_FILE'
SCOPES = ['openid', 'https://www.googleapis.com/auth/userinfo.email']


def GetDashboardToken(file: Optional[io.TextIOWrapper]) -> str:
  if file:
    service_account_info = json.load(file)
  else:
    key_path = os.environ.get(SERVICE_ACCOUNT_FILE_ENV_NAME)
    if key_path is None:
      raise RuntimeError(
          f'Can\'t get the token, set ENV {SERVICE_ACCOUNT_FILE_ENV_NAME}')
    with open(key_path, 'r') as f:
      service_account_info = json.load(f)
  credentials = service_account.Credentials.from_service_account_info(
      service_account_info, scopes=SCOPES)
  if credentials is None:
    raise RuntimeError('Error reading authentication token')
  credentials.refresh(Request())
  if credentials.token is None:
    raise RuntimeError('Can\'t get the token')
  return credentials.token


def main():
  log_level = logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)
  parser = argparse.ArgumentParser()
  parser.add_argument('--service_account_json_file',
                      type=argparse.FileType('r'),
                      help='json file with a service account private key')
  args = parser.parse_args()

  token = GetDashboardToken(args.service_account_json_file)
  print(token)


if __name__ == '__main__':
  sys.exit(main())
