# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
import logging
import os
import shutil
import json

from typing import Dict, List, Optional

import components.path_util as path_util

from components.perf_test_utils import GetProcessOutput

GH_BRAVE_CORE_GIT_URL = 'git@github.com:brave/brave-core.git'
GH_BRAVE_VARIATIONS_GIT_URL = 'git@github.com:brave/brave-variations.git'
GH_BRAVE_PERF_TEAM = 'brave/perf-team'


def DoesPrOpen(branch: str, target: Optional[str] = None):
  args = ['gh', 'pr', 'list', '--head', branch, '--json', 'number']
  if target is not None:
    args.extend(['--base', target])
  _, output = GetProcessOutput(args, cwd=path_util.GetBraveDir(), check=True)
  pr_list = json.loads(output)
  assert isinstance(pr_list, list)
  return len(pr_list) > 0


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
  # Make a few attempts to rebase if non fast-forward
  for attempt in range(3):
    logging.info('Pushing changes to branch %s #%d', branch, attempt)
    branch_exists, _ = GetProcessOutput(
        ['git', 'fetch', GH_BRAVE_CORE_GIT_URL, branch],
        cwd=path_util.GetBraveDir())
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
    success, _ = GetProcessOutput(
        ['git', 'push', GH_BRAVE_CORE_GIT_URL, f'{branch}:{branch}'],
        cwd=path_util.GetBraveDir())
    if success:
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


def Clone(url: str, branch: Optional[str], target_dir: str) -> None:
  args = ['git', 'clone', url, target_dir]
  if branch:
    args.extend(['--branch', branch])
  GetProcessOutput(args, path_util.GetBraveDir(), check=True)


def EnsureRepositoryUpdated(url: str, branch: str, directory: str) -> None:
  if not os.path.exists(directory):
    Clone(url, branch, directory)
    return

  GetProcessOutput(['git', 'fetch', url, branch], directory, check=True)
  GetProcessOutput(['git', 'checkout', 'FETCH_HEAD'], directory, check=True)
