# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
import logging
import os
import shutil

from typing import Dict, List, Optional

import components.path_util as path_util

from components.perf_test_utils import GetProcessOutput

GH_BRAVE_PERF_TEAM = 'brave/perf-team'


def MakeGithubPR(branch: str, target: str, title: str, body: str,
                 extra_args: List[str], reviewers: List[str]):
  args = [
      'gh', 'pr', 'create', '--base', target, '--head', branch, '--title',
      title, '--body', body
  ]
  for reviewer in reviewers:
    args.extend(['--reviewer', reviewer])
  for arg in extra_args:
    args.append(arg)
  return GetProcessOutput(args, cwd=path_util.GetBraveDir())


def PushChangesToBranch(files: Dict[str, str], branch: str,
                        commit_message: str):

  #TODO: clarify email and username
  GetProcessOutput(
      ['git', 'config', 'user.email', '"brave-builds+devops@brave.com"'],
      cwd=path_util.GetBraveDir(),
      check=True)
  GetProcessOutput(['git', 'config', 'user.name', '"brave-builds"'],
                   cwd=path_util.GetBraveDir(),
                   check=True)

  for attempt in range(3):
    logging.info('Pushing changes to branch %s #%d', branch, attempt)
    branch_exists, _ = GetProcessOutput(['git', 'fetch', 'origin', branch],
                                        cwd=path_util.GetBraveDir())
    logging.info(branch_exists)
    if branch_exists:
      GetProcessOutput(['git', 'checkout', '-f', 'FETCH_HEAD'],
                       cwd=path_util.GetBraveDir(),
                       check=True)

    GetProcessOutput(['git', 'checkout', '-B', branch],
                     cwd=path_util.GetBraveDir(),
                     check=True)
    for local_file, stage_path in files.items():
      assert os.path.isfile(local_file)
      shutil.copy(local_file, stage_path)
      GetProcessOutput(['git', 'add', stage_path],
                       cwd=path_util.GetBraveDir(),
                       check=True)

    GetProcessOutput(['git', 'commit', '-m', f'{commit_message}'],
                     cwd=path_util.GetBraveDir(),
                     check=True)
    if GetProcessOutput(['git', 'push', 'origin', f'{branch}:{branch}'],
                        cwd=path_util.GetBraveDir()):
      return

  raise RuntimeError(f'Can\'t push changes to branch {branch}')


def GetFileAtRevision(filepath: str, revision: str) -> Optional[str]:
  if os.path.isabs(filepath):
    filepath = os.path.relpath(filepath, path_util.GetBraveDir())
  normalized_path = filepath.replace('\\', '/')
  success, content = GetProcessOutput(
      ['git', 'show', f'{revision}:{normalized_path}'],
      cwd=path_util.GetBraveDir(),
      output_to_debug=False)
  return content if success else None
