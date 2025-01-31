# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

# pylint: disable=too-few-public-methods

import logging
import os
import re

from typing import List, Optional, Dict, Tuple
from enum import Enum

from components.field_trials import FieldTrialsMode, ParseFieldTrialsMode
from components.browser_type import BrowserType, ParseBrowserType
from components.version import BraveVersion


class ProfileRebaseType(Enum):
  NONE = 1
  OFFLINE = 2
  ONLINE = 3


def _ParseProfileRebaseType(rebase_type: str) -> ProfileRebaseType:
  if rebase_type == 'none':
    return ProfileRebaseType.NONE
  if rebase_type == 'offline':
    return ProfileRebaseType.OFFLINE
  if rebase_type == 'online':
    return ProfileRebaseType.ONLINE
  raise RuntimeError(f'Unknown ProfileRebaseType {rebase_type}')

class RunnerConfig:
  """A description of a browser configuration that is able to run tests."""
  version: Optional[BraveVersion] = None
  location: Optional[str] = None
  label: Optional[str] = None
  profile = 'clean'
  field_trials: Optional[FieldTrialsMode] = None
  extra_browser_args: List[str] = []
  extra_benchmark_args: List[str] = []
  browser_type: BrowserType
  dashboard_bot_name: Optional[str] = None
  save_artifacts = True
  profile_rebase = ProfileRebaseType.OFFLINE

  def __init__(self, json: Dict[str, str]):
    assert isinstance(json, dict)
    for key in json:
      if key == 'target':
        self.version, location = ParseTarget(json[key])
        if not 'location' in json:
          self.location = location
        continue
      if key == 'browser-type':
        self.browser_type = ParseBrowserType(json[key])
        continue
      if key == 'field-trials':
        self.field_trials = ParseFieldTrialsMode(json[key])
        continue
      if key == 'profile-rebase':
        self.profile_rebase = _ParseProfileRebaseType(json[key])
        continue
      key_ = key.replace('-', '_')
      if not hasattr(self, key_):
        raise RuntimeError(f'Unexpected {key} in configuration')
      setattr(self, key_, json[key])

def ParseTarget(target: str) -> Tuple[Optional[BraveVersion], str]:
  """
  Parse the version and location from the passed string `target`.
  target = [<version>:][<location>]
  <version> could be:
  1. Brave tag (i.e. v1.62.1);
  2. Git hash;
  3. empty (for comparing builds when you don't need it).
  """

  m = re.match(r'^(v\d+\.\d+\.\d+|\w+)(?::(.+)|$)', target)
  if not m:
    return None, target
  version = BraveVersion(m.group(1))
  location = m.group(2)
  logging.debug('Parsed version: %s, location : %s', version.to_string(),
                location)
  if location is not None:
    if not location.startswith('https://') and not os.path.exists(location):
      raise RuntimeError(f'Bad location {location} in target {target}')
  return version, location


class BenchmarkConfig:
  """A description of one benchmark that can be launched on some RunnerConfigs.
  """
  name: str
  pageset_repeat: int = 1
  stories: List[str]
  stories_exclude: List[str]
  extra_benchmark_args: List[str] = []
  extra_browser_args: List[str] = []

  def __init__(self, json: Optional[dict] = None):
    if not json:
      return
    assert isinstance(json, dict)
    self.name = json['name']
    if pageset_repeat := json.get('pageset-repeat'):
      self.pageset_repeat = pageset_repeat
    self.stories = json.get('stories') or []
    self.stories_exclude = json.get('stories-exclude') or []
    self.extra_benchmark_args = json.get('extra-benchmark-args') or []
    self.extra_browser_args = json.get('extra-browser-args') or []

class PerfConfig:
  """A config includes configurations & benchmarks that should be launched.

  Each benchmark is launched on each configuration.
  The class has 1-1 match to .json5 files used to setup tests.
  """
  runners: List[RunnerConfig]
  benchmarks: List[BenchmarkConfig]

  def __init__(self, json: dict):
    assert isinstance(json, dict)
    runner_configs_list: List[dict] = json['configurations']
    self.runners = []
    for runner_json in runner_configs_list:
      self.runners.append(RunnerConfig(runner_json))

    self.benchmarks = []
    benchmarks_list: List[dict] = json['benchmarks']
    for benchmark_json in benchmarks_list:
      self.benchmarks.append(BenchmarkConfig(benchmark_json))
