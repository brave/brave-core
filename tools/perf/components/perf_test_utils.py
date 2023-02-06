# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
import os
import subprocess
import logging

from typing import Tuple, List, Optional

import components.path_util as path_util

with path_util.SysPath(path_util.GetPyJson5Dir()):
  import json5  # pylint: disable=import-error


def GetProcessOutput(args: List[str], cwd: Optional[str] = None,
                     check=False) -> Tuple[bool, str]:
  try:
    logging.debug('Run binary: %s, cwd = %s', ' '.join(args), cwd)

    output = subprocess.check_output(args,
                                     stderr=subprocess.STDOUT,
                                     cwd=cwd,
                                     text=True)
    logging.debug('Binary output: %s', output)
    return True, output
  except subprocess.CalledProcessError as e:
    logging.error(e.output)
    if check:
      raise
    return False, e.output


def GetConfigPath(config_path: str) -> str:
  if os.path.isfile(config_path):
    return config_path

  config_path = os.path.join(path_util.GetBravePerfConfigDir(), config_path)
  if os.path.isfile(config_path):
    return config_path
  raise RuntimeError(f'Bad config {config_path}')


def LoadJsonConfig(config_path: str) -> dict:
  config_path = GetConfigPath(config_path)
  with open(config_path, 'r', encoding='utf-8') as config_file:
    return json5.load(config_file)


def GetRevisionNumberAndHash(revision: str) -> Tuple[str, str]:
  """Returns pair [revision_number, sha1]. revision_number is a number "primary"
  commits from the begging to `revision`.
  Use this to get the commit from a revision number:
  git rev-list --topo-order --first-parent --reverse origin/master
  | head -n <rev_num> | tail -n 1 | git log -n 1 --stdin
  """

  # Fetch the revision first:
  GetProcessOutput(['git', 'fetch', 'origin', revision],
                   cwd=path_util.GetBraveDir(),
                   check=True)

  # Fetch git hash of the revision:
  _, git_hash_output = GetProcessOutput(['git', 'rev-parse', 'FETCH_HEAD'],
                                        cwd=path_util.GetBraveDir(),
                                        check=True)

  rev_number_args = [
      'git', 'rev-list', '--topo-order', '--first-parent', '--count',
      'FETCH_HEAD'
  ]

  # Get the revision number:
  _, rev_number_output = GetProcessOutput(rev_number_args,
                                          cwd=path_util.GetBraveDir(),
                                          check=True)

  return (rev_number_output.rstrip(), git_hash_output.rstrip())
