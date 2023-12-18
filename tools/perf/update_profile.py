#!/usr/bin/env python3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
""" A script to automate updating the perf profiles for perf CI

It downloads and unpacks an old profile, run the browser twice to upload the
components/other things, makes a new zip and uploads it to GCP.

Example usage:
vpython3 update_profile.py brave-typical-v1.46.52 brave-typical-mac-v1.55.98
  "/Applications/Brave Browser Nightly.app/Contents/MacOS/Brave Browser Nightly"
"""

import argparse
import json
import logging
import os
import subprocess
import sys
import tempfile
import time

import components.path_util as path_util
import components.perf_profile as perf_profile

with path_util.SysPath(path_util.GetBraveScriptDir(), 0):
  from lib.util import make_zip, scoped_cwd


def EraseVariationsFromLocalState(local_state_path: str):
  local_state = {}
  with open(local_state_path, 'r', encoding='utf8') as f:
    local_state = json.load(f)
    for k in list(local_state.keys()):
      if k.startswith('variation'):
        local_state.pop(k, None)

  with open(local_state_path, 'w', encoding='utf8') as f:
    json.dump(local_state, f)


def UpdateProfile(profile_name: str, browser_binary: str, new_profile_name: str,
                  skip_upload: bool):
  assert os.path.exists(browser_binary)
  tmp = tempfile.mkdtemp(prefix='')
  logging.info('Using temp directory %s', tmp)
  profile_dir = perf_profile.GetProfilePath(profile_name, tmp)
  logging.info('Profile unpacked in %s', profile_dir)

  args = [
      browser_binary,
      f'--user-data-dir={profile_dir}',

      # Enable perf related features to download the components
      '--enable-brave-features-for-perf-testing',

      # Speed up component updating
      '--component-updater="fast-update"'
  ]

  # Run browser for 2 minutes to update the profile.
  # Do this twice to clean the old components.
  for _ in range(2):
    logging.info('Run %s', ' '.join(args))
    with subprocess.Popen(args) as process:
      logging.info('Waiting for 2 minutes..')
      time.sleep(120)
      if sys.platform == 'win32':
        logging.info('Please close the browser')
        process.wait()
      else:
        logging.info('Closing the browser')
        process.terminate()
      time.sleep(5)

  # Secure Preferences can't be used on another machine.
  os.remove(os.path.join(profile_dir, 'Default', 'Secure Preferences'))

  # We don't want any preloaded variations
  EraseVariationsFromLocalState(os.path.join(profile_dir, 'Local State'))

  zip_path = os.path.join(path_util.GetBravePerfProfileDir(),
                          new_profile_name + '.zip')
  logging.info('Creating zipped profile %s', zip_path)
  with scoped_cwd(profile_dir):
    make_zip(zip_path, [], ['.'])

  upload_args = [
      sys.executable,
      os.path.join(path_util.GetDepotToolsDir(), 'upload_to_google_storage.py'),
      '-b', 'brave-telemetry', zip_path
  ]
  if skip_upload:
    logging.info('Upload skipped, call this to do: %s', ' '.join(upload_args))
  else:
    subprocess.check_call(upload_args)
    logging.info('Please run git add %s.sha1', zip_path)


def main():
  log_level = logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)
  parser = argparse.ArgumentParser()
  parser.add_argument('profile_name',
                      type=str,
                      help='Profile name to update (a filename in ./profiles)')
  parser.add_argument('new_profile_name',
                      type=str,
                      help='New profile name to store the result')
  parser.add_argument('browser_binary',
                      type=str,
                      help='A browser binary used to update')
  parser.add_argument('--skip-upload',
                      action='store_true',
                      help='Skip uploading to GCP')
  args = parser.parse_args()

  UpdateProfile(args.profile_name, args.browser_binary, args.new_profile_name,
                args.skip_upload)


if __name__ == '__main__':
  sys.exit(main())
