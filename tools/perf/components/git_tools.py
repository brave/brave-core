import logging
import os
import shutil

from typing import Dict, Optional

import components.path_util as path_util

from components.perf_test_utils import GetProcessOutput


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
      return True

  return False


def GetFileAtRevision(filepath: str, revision: str) -> Optional[str]:
  if os.path.isabs(filepath):
    filepath = os.path.relpath(filepath, path_util.GetBraveDir())
  normalized_path = filepath.replace('\\', '/')
  success, content = GetProcessOutput(
      ['git', 'show', f'{revision}:{normalized_path}'],
      cwd=path_util.GetBraveDir(),
      output_to_debug=False)
  return content if success else None
