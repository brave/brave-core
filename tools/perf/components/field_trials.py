# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import base64
from dataclasses import dataclass
from enum import Enum
import gzip
import json
import os
import sys
import tempfile
from typing import Optional

from components.perf_test_utils import GetProcessOutput
from components.version import BraveVersion
import components.git_tools as git_tools


class FieldTrialsMode(Enum):
  NO_TRIALS = 1
  GRIFFIN = 2
  TESTING_FIELD_TRIALS = 3


@dataclass
class FieldTrialConfig:
  mode: FieldTrialsMode
  seed: Optional[bytes]
  revision: str
  fake_channel: Optional[str] = None


def ParseFieldTrialsMode(string_type: str) -> FieldTrialsMode:
  if string_type in ('no-trials', ''):  # Default
    return FieldTrialsMode.NO_TRIALS
  if string_type == 'griffin':
    return FieldTrialsMode.GRIFFIN
  if string_type == 'testing-field-trials':
    return FieldTrialsMode.TESTING_FIELD_TRIALS
  raise RuntimeError('Bad field trial mode ' + string_type)


def MakeFieldTrials(mode: FieldTrialsMode, artifacts_dir: str,
                    version: Optional[BraveVersion],
                    variations_repo_dir: Optional[str]) -> FieldTrialConfig:
  if mode != FieldTrialsMode.GRIFFIN:
    return FieldTrialConfig(mode, None, '')

  if version is None:
    raise RuntimeError('Using Griffin requires a version')

  if variations_repo_dir is None:
    variations_repo_dir = os.path.join(tempfile.gettempdir(),
                                       'brave-variations')
    git_tools.EnsureRepositoryUpdated(git_tools.GH_BRAVE_VARIATIONS_GIT_URL,
                                      'main', variations_repo_dir)

  sha1 = git_tools.GetRevisionFromDate(version.commit_date, 'main',
                                       variations_repo_dir)

  seed_path = os.path.join(artifacts_dir, 'seed.bin')

  npm = 'npm.cmd' if sys.platform == 'win32' else 'npm'
  GetProcessOutput([npm, 'install'], cwd=variations_repo_dir, check=True)

  args = [npm, 'run', 'seed_tools', 'create', '--'] + [
      'studies', seed_path, '--perf_mode'
  ] + ['--revision', sha1] + ['--version', f'perf@{sha1}']

  GetProcessOutput(args, cwd=variations_repo_dir, check=True)

  assert os.path.isfile(seed_path)
  with open(seed_path, 'rb') as seed_file:
    seed = seed_file.read()
  return FieldTrialConfig(mode, seed, sha1)


def MaybeInjectSeedToLocalState(field_trial_config: FieldTrialConfig,
                                profile_dir: Optional[str]) -> None:
  if field_trial_config.mode != FieldTrialsMode.GRIFFIN:
    return
  assert field_trial_config.seed

  if profile_dir is None:
    raise RuntimeError('Using Griffin requires a non-empty profile')

  local_state_path = os.path.join(profile_dir, 'Local State')
  if os.path.isfile(local_state_path):
    with open(local_state_path, 'r', encoding='utf8') as f:
      local_state = json.load(f)
  else:
    local_state = {}

  # Gzip the seed and encode it as base64
  gzipped_seed = gzip.compress(field_trial_config.seed)
  gzipped_seed_base64 = base64.b64encode(gzipped_seed).decode('utf-8')

  local_state['variations_compressed_seed'] = "safe_seed_content"
  local_state['variations_safe_compressed_seed'] = gzipped_seed_base64

  with open(local_state_path, 'w', encoding='utf8') as f:
    json.dump(local_state, f, indent=2)
