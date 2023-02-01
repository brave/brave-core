# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

# pylint: disable=too-few-public-methods

from typing import List, Optional, Dict

from components.browser_binary_fetcher import ParseTarget
from components.browser_type import BrowserType, BraveVersion, ParseBrowserType


class RunnerConfig:
  """A description of a browser configuration that is able to run tests."""
  tag: Optional[BraveVersion] = None
  location: Optional[str] = None
  label: Optional[str] = None
  profile = 'clean'
  rebase_profile = True
  extra_browser_args: List[str] = []
  extra_benchmark_args: List[str] = []
  browser_type: BrowserType
  dashboard_bot_name: Optional[str] = None
  save_artifacts = False

  def __init__(self, json: Dict[str, str]):
    assert isinstance(json, dict)
    for key in json:
      if key == 'target':
        self.tag, location = ParseTarget(json[key])
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
    self.pageset_repeat = json['pageset-repeat']
    self.stories = []
    if 'stories' in json:
      story_list: List[str] = json['stories']
      for story in story_list:
        self.stories.append(story)


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
