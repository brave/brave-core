# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

# pylint: disable=too-few-public-methods

import logging
import re

from typing import List, Optional, Dict, Tuple

from components.browser_type import BrowserType, ParseBrowserType
from components.version import BraveVersion


class RunnerConfig:
  """A description of a browser configuration that is able to run tests."""
  version: Optional[BraveVersion] = None
  location: Optional[str] = None
  label: Optional[str] = None
  profile = 'clean'
  extra_browser_args: List[str] = []
  extra_benchmark_args: List[str] = []
  browser_type: BrowserType
  dashboard_bot_name: Optional[str] = None
  save_artifacts = True

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
  return version, location


class BenchmarkConfig:
  """A description of one benchmark that can be launched on some RunnerConfigs.
  """
  name: str
  pageset_repeat: int = 1
  stories: List[str]

  def __init__(self, json: Optional[dict] = None):
    if not json:
      return
    assert isinstance(json, dict)
    self.name = json['name']
    if pageset_repeat := json.get('pageset-repeat'):
      self.pageset_repeat = pageset_repeat
    self.stories = []
    if story_list := json.get('stories'):
      self.stories.extend(story_list)


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
