# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import logging
import os
import re
import shutil
import json

from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict

import components.cloud_storage as cloud_storage
import components.git_tools as git_tools

from components.path_util import GetBravePerfProfileDir
from components.perf_profile import GetProfilePath
from components.perf_config import RunnerConfig
from components.common_options import CommonOptions

from lib.util import make_zip, scoped_cwd

_CACHE_DIRECTORIES = [
    os.path.join('Default', 'Cache'),
    os.path.join('Default', 'Code Cache'),
    os.path.join('Default', 'GPUCache'), 'cache', 'GrShaderCache',
    'GraphiteDawnCache', 'ShaderCache', 'component_crx_cache'
]

_PR_SEE_DETAILS_LINK = ('https://github.com/brave/brave-core/blob/master/' +
                        'tools/perf/updating_test_profiles.md')
_PR_BODY = f"""Automated perf profile update via CI
Pre-approval checklist:
- Wait all expected profiles are update (currently 6 profiles).
- Review the changes in .size files.

Notes:
* Multiple pushes are expected after the PR is created.
* Until the PR is merged it has no effect, so CI/skip is added.

[See details]({_PR_SEE_DETAILS_LINK})"""


def _GetDirectorySize(path: str):
  return sum(f.stat().st_size for f in Path(path).glob('**/*') if f.is_file())


def _GetComponentInfo(path: str) -> Optional[Tuple[str, str]]:
  if not os.path.isdir(path):
    return None

  for f in Path(path).glob('**/manifest.json'):
    with open(f, 'r') as f:
      manifest_json = json.load(f)
      if 'name' in manifest_json and 'version' in manifest_json:
        return manifest_json['name'], manifest_json['version']
  return None


def _EraseVariationsFromLocalState(local_state_path: str):
  local_state = {}
  with open(local_state_path, 'r', encoding='utf8') as f:
    local_state = json.load(f)
    for k in list(local_state.keys()):
      if k.startswith('variation'):
        local_state.pop(k, None)

  with open(local_state_path, 'w', encoding='utf8') as f:
    json.dump(local_state, f)


def CleanupBrowserComponents(cfg: RunnerConfig, options: CommonOptions):
  assert cfg.version is not None
  profile_dir = GetProfilePath(cfg.profile, options.working_directory,
                               cfg.version)
  for f in os.listdir(profile_dir):
    fullpath = os.path.join(profile_dir, f)
    if _GetComponentInfo(fullpath) is not None:
      # looks like a component, remove it to re download if needed
      shutil.rmtree(fullpath, ignore_errors=True)


def CleanProfileCaches(profile_dir: str):
  for path in _CACHE_DIRECTORIES:
    shutil.rmtree(os.path.join(profile_dir, path), ignore_errors=True)


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

  CleanProfileCaches(profile_dir)

  zip_filename = cfg.profile + '.zip'
  sizes_filename = cfg.profile + '.zip.sizes'

  profile_zip = os.path.join(options.working_directory, 'artifacts',
                             zip_filename)
  profile_zip_sizes = os.path.join(options.working_directory, 'artifacts',
                                   sizes_filename)
  logging.info('Packing profile %s to %s', profile_dir, profile_zip)
  with scoped_cwd(profile_dir):
    make_zip(profile_zip, files=[], dirs=['.'])

  with open(profile_zip_sizes, 'w', encoding='utf-8') as f:
    f.write(GetProfileStats(profile_dir).toText())

  if options.upload:
    new_profile_sha1_path = cloud_storage.UploadFileToCloudStorage(
        cloud_storage.CloudFolder.TEST_PROFILES, profile_zip)
    files: Dict[str, str] = dict()
    files[new_profile_sha1_path] = os.path.join(GetBravePerfProfileDir(),
                                                zip_filename + '.sha1')
    files[profile_zip_sizes] = os.path.join(GetBravePerfProfileDir(),
                                            sizes_filename)
    version_str = cfg.version.to_string()
    commit_message = f'Update perf profile {cfg.profile} using {version_str}'
    branch = options.upload_branch or f'update-perf-profiles-{version_str}'
    git_tools.PushChangesToBranch(files, branch, commit_message)

    if git_tools.DoesPrOpen(branch):
      logging.info('The PR exists, skip making it')
    else:
      logging.info('Making a github PR from branch %s', branch)
      git_tools.MakeGithubPR(branch=branch,
                             target='master',
                             title=f'Roll perf profiles update ({branch})',
                             body=_PR_BODY,
                             reviewers=[git_tools.GH_BRAVE_PERF_TEAM],
                             extra_args=['--label', 'CI/skip'])



def _sizeKB(size: int) -> int:
  return int(size / 1024)


@dataclass
class StatItem:
  path: str
  size: int
  name: Optional[str] = None
  version: Optional[str] = None

  def __lt__(self, other):
    return self.path < other.path


class GroupStat:
  name: str
  items: List[StatItem]

  def __init__(self):
    self.items = []

  def size(self) -> int:
    return sum(item.size for item in self.items)

  def toJSON(self) -> Dict:
    return {
        'items': [asdict(item) for item in sorted(self.items)],
        'group_size': self.size()
    }


class ProfileStats:
  total_size: int = 0
  groups: Dict[str, GroupStat] = {}

  def toJSON(self) -> Dict:
    return {
        'total_size': self.total_size,
        'groups': {
            key: group.toJSON()
            for key, group in sorted(self.groups.items())
        }
    }

  def toText(self) -> str:
    result = f'Total size {_sizeKB(self.total_size)}K\n'
    for name, group in sorted(self.groups.items()):
      result += f'\nGROUP {name} [{_sizeKB(group.size())}K total]\n'
      for item in sorted(group.items):
        if item.name is not None and item.version is not None:
          text = f'{item.name} {item.version} [{item.path}]'
        else:
          text = item.path
        result += '{:6}K {}\n'.format(_sizeKB(item.size), text)
    return result


def _GetComponentGroup(name: str, path: str,
                       skip_chromium_components) -> Optional[str]:
  if 'Ad Block' in name or 'Adblock' in name:
    return 'Adblock'
  if 'Ads' in name:
    return 'Ads'
  if 'NTP' in name:
    return 'Ntp'
  if 'Wallet' in name:
    return 'Wallet'
  if 'Brave' in name or re.match(r'[a-z]{32}', path) is not None:
    return 'Other'
  return None if skip_chromium_components else 'chromium'


def GetProfileStats(profile_dir: str,
                    skip_chromium_components=False) -> ProfileStats:
  result = ProfileStats()

  with scoped_cwd(profile_dir):
    result.total_size = _GetDirectorySize(profile_dir)

    for f in os.listdir(profile_dir):
      info = _GetComponentInfo(f)
      if info is not None:
        name, version = info
        group = _GetComponentGroup(name, f, skip_chromium_components)
        if group is None:
          continue
        size = _GetDirectorySize(f)
        item = StatItem(path=f, size=size, name=name, version=version)
        result.groups.setdefault(group, GroupStat()).items.append(item)
      else:
        for file in Path(f).glob('**/*'):
          is_cache = any(str(file).startswith(c) for c in _CACHE_DIRECTORIES)
          if is_cache or not file.is_file():
            continue
          size = file.stat().st_size
          if size > 1000 * 1000:
            item = StatItem(path=str(file), size=size)
            result.groups.setdefault('large_files',
                                     GroupStat()).items.append(item)

  return result
