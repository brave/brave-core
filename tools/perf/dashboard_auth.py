#!/usr/bin/env vpython3
#
# [VPYTHON:BEGIN]
# wheel: <
#   name: "infra/python/wheels/google-auth-py2_py3"
#   version: "version:1.2.1"
# >
#
# wheel: <
#   name: "infra/python/wheels/pyasn1-py2_py3"
#   version: "version:0.4.5"
# >
#
# wheel: <
#   name: "infra/python/wheels/pyasn1_modules-py2_py3"
#   version: "version:0.2.4"
# >
#
# wheel: <
#   name: "infra/python/wheels/six"
#   version: "version:1.10.0"
# >
#
# wheel: <
#   name: "infra/python/wheels/cachetools-py2_py3"
#   version: "version:2.0.1"
# >
# wheel: <
#   name: "infra/python/wheels/rsa-py2_py3"
#   version: "version:4.0"
# >
#
# wheel: <
#   name: "infra/python/wheels/requests"
#   version: "version:2.13.0"
# >
#
# wheel: <
#   name: "infra/python/wheels/google-api-python-client-py2_py3"
#   version: "version:1.6.2"
# >
#
# wheel: <
#   name: "infra/python/wheels/httplib2-py2_py3"
#   version: "version:0.12.1"
# >
#
# wheel: <
#   name: "infra/python/wheels/oauth2client-py2_py3"
#   version: "version:3.0.0"
# >
#
# wheel: <
#   name: "infra/python/wheels/uritemplate-py2_py3"
#   version: "version:3.0.0"
# >
#
# wheel: <
#   name: "infra/python/wheels/google-auth-oauthlib-py2_py3"
#   version: "version:0.3.0"
# >
#
# wheel: <
#   name: "infra/python/wheels/requests-oauthlib-py2_py3"
#   version: "version:1.2.0"
# >
#
# wheel: <
#   name: "infra/python/wheels/oauthlib-py2_py3"
#   version: "version:3.0.1"
# >
#
# wheel: <
#   name: "infra/python/wheels/google-auth-httplib2-py2_py3"
#   version: "version:0.0.3"
# >
# [VPYTHON:END]
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
"""A tool to perform authorization to https://brave-perf-dashboard.appspot.com/

The tool performs OAuth2 flow, stores/loads credentials, refreshes a token.
Returns a valid token to stdout if succeeded.
"""

import os
import sys
import json

# pylint: disable=no-name-in-module,import-error
from components import path_util
with path_util.SysPath(path_util.GetGoogleAuthDir(), 0):
  from google.auth.transport.requests import Request
  from google.oauth2.credentials import Credentials
  from google_auth_oauthlib.flow import InstalledAppFlow
# pylint: enable=no-name-in-module,import-error

SCOPES = ['openid', 'https://www.googleapis.com/auth/userinfo.email']
CLIENT_ID_FILE = os.path.join(os.path.expanduser("~"),
                              '.perf_dashboard_client_id.json')
PERF_CREDENTIAL_FILE = os.path.join(os.path.expanduser("~"),
                                    '.perf_dashboard_credentials.json')


def to_json(credentials):
  prep = {
      "token": credentials.token,
      "refresh_token": credentials.refresh_token,
      "token_uri": credentials.token_uri,
      "client_id": credentials.client_id,
      "client_secret": credentials.client_secret,
      "scopes": credentials.scopes,
  }
  if credentials.expiry:  # flatten expiry timestamp
    prep["expiry"] = credentials.expiry.isoformat() + "Z"

  # Remove empty entries (those which are None)
  prep = {k: v for k, v in prep.items() if v is not None}

  return json.dumps(prep)


def _GetDashboardCredentials(can_be_interactive=False):
  credentials = None
  should_store_new_credentials = False

  if os.path.exists(PERF_CREDENTIAL_FILE):
    credentials = Credentials.from_authorized_user_file(PERF_CREDENTIAL_FILE,
                                                        SCOPES)

  if credentials and not credentials.valid and credentials.refresh_token:
    credentials.refresh(Request())
    should_store_new_credentials = True

  if not credentials or not credentials.valid or credentials.expired:
    if can_be_interactive:
      flow = InstalledAppFlow.from_client_secrets_file(CLIENT_ID_FILE, SCOPES)
      credentials = flow.run_console()
      should_store_new_credentials = True
    else:
      raise RuntimeError('No valid credentials for brave-perf-dashboard')

  if should_store_new_credentials:
    with open(PERF_CREDENTIAL_FILE, 'w') as credentials_dat:
      credentials_dat.write(to_json(credentials))
  return credentials


def GetDashboardToken(can_be_interactive=False):
  credentials = _GetDashboardCredentials(can_be_interactive)
  if credentials is None:
    raise RuntimeError('Error generating authentication token')
  return credentials.token


def main():
  token = GetDashboardToken(can_be_interactive=True)
  print(token)


if __name__ == '__main__':
  sys.exit(main())
