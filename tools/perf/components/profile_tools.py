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
import zipfile

from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict

import components.cloud_storage as cloud_storage
import components.git_tools as git_tools
import components.perf_test_runner as perf_test_runner
import components.perf_config as perf_config

from components.path_util import GetBravePerfProfileDir
from components.perf_profile import GetProfilePath
from components.perf_config import RunnerConfig
from components.common_options import CommonOptions, PerfMode

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


def _GetSafeBrowsingDir(profile_dir: str) -> str:
  return os.path.join(profile_dir, 'Safe Browsing')


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


def PreRebaseCleanup(cfg: RunnerConfig, options: CommonOptions):
  assert cfg.version is not None
  profile_dir = GetProfilePath(cfg.profile, options.working_directory,
                               cfg.version)

  # Remove all components to re download them
  for f in os.listdir(profile_dir):
    fullpath = os.path.join(profile_dir, f)
    if _GetComponentInfo(fullpath) is not None:
      # looks like a component, remove it to re download if needed
      shutil.rmtree(fullpath, ignore_errors=True)

  # Remove safe browsing database
  shutil.rmtree(_GetSafeBrowsingDir(profile_dir), ignore_errors=True)

  _FixupPreferences(profile_dir)


def CleanProfileCaches(profile_dir: str):
  for path in _CACHE_DIRECTORIES:
    shutil.rmtree(os.path.join(profile_dir, path), ignore_errors=True)


def _FixupPreferences(profile_dir: str):
  """Cleanup possible invalid entries in Preferences file"""

  preferences_path = os.path.join(profile_dir, 'Default', 'Preferences')
  if not os.path.isfile(preferences_path):
    return

  # Cleanup Brave News preferences. They could vary depending on the locale.
  with open(preferences_path, 'r', encoding='utf8') as f:
    preferences = json.load(f)
    if brave := preferences.get('brave'):
      brave.pop('news', None)
      if new_tab_page := brave.get('new_tab_page'):
        new_tab_page.pop('show_brave_news', None)

  with open(preferences_path, 'w', encoding='utf8') as f:
    json.dump(preferences, f)

def MakeUpdatedProfileArchive(cfg: RunnerConfig, options: CommonOptions,
                              extra_dirs_to_add: List[str]) -> str:
  assert cfg.version is not None
  profile_dir = GetProfilePath(cfg.profile, options.working_directory,
                               cfg.version)

  # Secure Preferences can't be used on another machine.
  secure_prefs_path = os.path.join(profile_dir, 'Default', 'Secure Preferences')
  if os.path.isfile(secure_prefs_path):
    os.remove(secure_prefs_path)

  # Ads service state should be sync with Secure Preferences.
  ads_service_path = os.path.join(profile_dir, 'Default', 'ads_service')
  shutil.rmtree(ads_service_path, ignore_errors=True)

  # We don't want any preloaded variations
  _EraseVariationsFromLocalState(os.path.join(profile_dir, 'Local State'))

  CleanProfileCaches(profile_dir)

  zip_filename = cfg.profile + '.zip'
  sizes_filename = cfg.profile + '.zip.sizes'

  profile_zip = os.path.join(options.working_directory, 'artifacts',
                             zip_filename)
  profile_zip_sizes = os.path.join(options.working_directory, 'artifacts',
                                   sizes_filename)

  for extra_dir in extra_dirs_to_add:
    target_dir = os.path.join(profile_dir, os.path.basename(extra_dir))
    logging.info('Adding extra %s to %s', extra_dir, target_dir)
    shutil.copytree(extra_dir, target_dir, dirs_exist_ok=True)

  logging.info('Packing profile %s to %s', profile_dir, profile_zip)
  # strict_timestamps=False because Chromium makes files with empty timestamps.
  with zipfile.ZipFile(profile_zip,
                       "w",
                       zipfile.ZIP_DEFLATED,
                       strict_timestamps=False) as zip_file:
    with scoped_cwd(profile_dir):
      for root, _, filenames in os.walk('.'):
        for f in filenames:
          print('adding', os.path.join(root, f))
          zip_file.write(os.path.join(root, f))

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
  return profile_dir



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

  def __init__(self):
    self.total_size: int = 0
    self.groups: Dict[str, GroupStat] = {}

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


def RunUpdateProfile(brave_config: perf_config.PerfConfig,
                     chromium_config: perf_config.PerfConfig,
                     options: CommonOptions) -> bool:
  brave_profile = _RunUpdateProfileForConfig(brave_config, options, [])
  if not brave_profile:
    return False

  # Take Safe Browsing database from Brave profile
  safe_browsing = _GetSafeBrowsingDir(brave_profile)
  extra_dirs_to_add = []
  if os.path.isdir(safe_browsing):
    extra_dirs_to_add.append(safe_browsing)

  if not _RunUpdateProfileForConfig(chromium_config, options,
                                    extra_dirs_to_add):
    return False

  return True


def _RunUpdateProfileForConfig(config: perf_config.PerfConfig,
                               options: CommonOptions,
                               extra_dirs_to_add: List[str]) -> Optional[str]:
  if len(config.runners) != 1:
    raise RuntimeError('Only one configuration should be specified.')
  options.do_report = False
  runner = config.runners[0]
  runner.profile_rebase = perf_config.ProfileRebaseType.NONE

  # Remove --disable-component-update to get all the components
  runner.extra_browser_args = [
      arg for arg in runner.extra_browser_args
      if arg != '--disable-component-update'
  ]

  def make_benchmark_config(delay: int):
    return perf_config.BenchmarkConfig({
        'name':
        'brave_utils.online',
        'pageset-repeat':
        1,
        'stories': ['UpdateProfile'],
        'stories_exclude': [],
        'extra-benchmark-args': [f'--delay={delay}'],
    })

  config.benchmarks = [
      # 15 minutes to update everything
      make_benchmark_config(15 * 60),

      # two short runs to drop old files
      make_benchmark_config(30),
      make_benchmark_config(30),
  ]

  configurations = perf_test_runner.SpawnConfigurationsFromTargetList(
      options.targets, runner)
  assert len(configurations) == 1
  PreRebaseCleanup(configurations[0], options)
  if not perf_test_runner.RunConfigurations(configurations, config.benchmarks,
                                            options):
    return None

  return MakeUpdatedProfileArchive(configurations[0], options,
                                   extra_dirs_to_add)
