# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import logging
import os
import re
import shutil

from components.perf_profile import GetProfilePath
from components.perf_config import RunnerConfig
from components.common_options import CommonOptions

from lib.util import make_zip, scoped_cwd

def _EraseVariationsFromLocalState(local_state_path: str):
  local_state = {}
  with open(local_state_path, 'r', encoding='utf8') as f:
    local_state = json.load(f)
    for k in list(local_state.keys()):
      if k.startswith('variation'):
        local_state.pop(k, None)

  with open(local_state_path, 'w', encoding='utf8') as f:
    json.dump(local_state, f)


def CleanupBeforeRun(cfg: RunnerConfig, options: CommonOptions):
  assert cfg.version is not None
  profile_dir = GetProfilePath(cfg.profile, options.working_directory,
                               cfg.version)
  for f in os.listdir(profile_dir):
    fullpath = os.path.join(profile_dir, f)
    if os.path.isdir(fullpath) and re.match(r'[a-z]{32}', f) is not None:
      # looks like a component, remove it to re download if needed
      shutil.rmtree(fullpath, ignore_errors=True)


def MakeUpdatedProfileArchive(cfg: RunnerConfig, options: CommonOptions):
  assert cfg.version is not None
  profile_dir = GetProfilePath(cfg.profile, options.working_directory,
                               cfg.version)

  # Secure Preferences can't be used on another machine.
  secure_prefs_path = os.path.join(profile_dir, 'Default', 'Secure Preferences')
  if os.path.isfile(secure_prefs_path):
    os.remove(secure_prefs_path)

  # We don't want any preloaded variations
  _EraseVariationsFromLocalState(os.path.join(profile_dir, 'Local State'))

  profile_zip = os.path.join(options.working_directory, 'artifacts',
                             cfg.profile + '.zip')
  logging.info('Packing profile %s to %s', profile_dir, profile_zip)
  with scoped_cwd(profile_dir):
    make_zip(profile_zip, files=[], dirs=['.'])
