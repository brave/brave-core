# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
import argparse
import os
import json
import logging
import shutil
import time

from copy import deepcopy
from typing import Tuple, Optional, List

import components.path_util as path_util
import components.perf_profile as perf_profile
import components.perf_test_utils as perf_test_utils

from components.perf_config import BenchmarkConfig, RunnerConfig
from components.browser_type import BrowserType
from components.browser_binary_fetcher import PrepareBinary, ParseTarget


class CommonOptions:
  verbose: bool = False
  variations_repo_dir: Optional[str] = None
  working_directory: str = ''

  do_run_tests: bool = True
  do_report: bool = False
  report_on_failure: bool = False
  local_run: bool = False

  @classmethod
  def add_common_parser_args(cls, parser: argparse.ArgumentParser) -> None:
    parser.add_argument('--verbose', action='store_true')
    parser.add_argument('--variations-repo-dir', type=str)
    parser.add_argument('--working-directory', required=True, type=str)

  @classmethod
  def from_args(cls, args) -> 'CommonOptions':
    options = CommonOptions()
    options.verbose = args.verbose
    options.variations_repo_dir = args.variations_repo_dir
    options.working_directory = args.working_directory
    return options


def ReportToDashboardImpl(browser_type: BrowserType, dashboard_bot_name: str,
                          revision: str, output_dir: str
                          ) -> Tuple[bool, List[str], Optional[str]]:

  if browser_type.ReportAsReference():
    # .reference suffix for benchmark folder is used in process_perf_results.py
    # to guess should the data be reported as reference or not.
    # Find and rename all benchmark folders to .reference format.
    for f in os.scandir(output_dir):
      if f.is_dir():
        for benchmark in os.scandir(f.path):
          if benchmark.is_dir() and not benchmark.name.endswith('.reference'):
            shutil.move(benchmark.path, benchmark.path + '.reference')

  args = [
      path_util.GetVpython3Path(),
      os.path.join(path_util.GetChromiumPerfDir(), 'process_perf_results.py')
  ]
  args.append(f'--configuration-name={dashboard_bot_name}')
  args.append(f'--task-output-dir={output_dir}')
  args.append('--output-json=' + os.path.join(output_dir, 'results.json'))

  revision_number, git_hash = perf_test_utils.GetRevisionNumberAndHash(revision)
  logging.debug('Got revision %s git_hash %s', revision_number, git_hash)

  build_properties = {}
  build_properties['bot_id'] = 'test_bot'
  build_properties['builder_group'] = 'brave.perf'

  build_properties['parent_builder_group'] = 'chromium.linux'
  build_properties['parent_buildername'] = 'Linux Builder'
  build_properties['recipe'] = 'chromium'
  build_properties['slavename'] = 'test_bot'

  # keep in sync with _MakeBuildStatusUrl() to make correct build urls.
  build_properties['buildername'] = (browser_type.GetName()) + '/' + revision
  build_properties['buildnumber'] = '001'

  build_properties[
      'got_revision_cp'] = 'refs/heads/main@{#%s}' % revision_number
  build_properties['got_v8_revision'] = revision_number
  build_properties['got_webrtc_revision'] = revision_number
  build_properties['git_revision'] = git_hash
  build_properties_serialized = json.dumps(build_properties)
  args.append('--build-properties=' + build_properties_serialized)

  success, _ = perf_test_utils.GetProcessOutput(args)
  if success:
    return True, [], revision_number

  return False, ['Reporting ' + revision + ' failed'], None


# pylint: disable=too-many-instance-attributes
class RunableConfiguration:
  common_options: CommonOptions
  benchmarks: List[BenchmarkConfig]
  config: RunnerConfig
  binary_path: Optional[str] = None
  out_dir: str
  profile_dir: Optional[str] = None
  field_trial_config: Optional[str] = None

  status_line: str = ''
  logs: List[str] = []

  def __init__(self, config: RunnerConfig, benchmarks: List[BenchmarkConfig],
               binary_path: Optional[str], out_dir: str,
               common_options: CommonOptions,
               field_trial_config: Optional[str]):
    self.config = config
    self.benchmarks = benchmarks
    self.binary_path = binary_path
    self.out_dir = out_dir
    self.common_options = common_options
    self.field_trial_config = field_trial_config

  def PrepareProfile(self) -> bool:
    start_time = time.time()
    if self.config.profile != 'clean':
      self.profile_dir = perf_profile.GetProfilePath(
          self.config.profile, self.common_options.working_directory)
      if self.config.rebase_profile:
        if not self.RebaseProfile():
          return False
    self.status_line += f'Prepare {(time.time() - start_time):.2f}s '
    return True

  def RebaseProfile(self) -> bool:
    assert (self.binary_path is not None)
    logging.info('Rebasing dir %s using binary %s', self.profile_dir,
                 self.binary_path)
    rebase_runner_config = deepcopy(self.config)
    rebase_runner_config.extra_browser_args = ['--update-source-profile']

    rebase_benchmark = BenchmarkConfig()
    rebase_benchmark.name = 'loading.desktop.brave'
    rebase_benchmark.stories = ['BraveSearch_cold']
    rebase_benchmark.pageset_repeat = 1

    return self.RunSingleTest(rebase_runner_config, rebase_benchmark, None,
                              True)

  def RunSingleTest(self, config: RunnerConfig,
                    benchmark_config: BenchmarkConfig, out_dir: Optional[str],
                    local_run: bool) -> bool:
    args = [path_util.GetVpython3Path()]
    args.append(os.path.join(path_util.GetChromiumPerfDir(), 'run_benchmark'))

    benchmark_name = benchmark_config.name

    if local_run:
      args.append(benchmark_name)
      if out_dir:
        assert (config.label is not None)
        args.append('--results-label=' + config.label)
        args.append(f'--output-dir={out_dir}')
      else:
        args.append('--output-format=none')

    else:
      args.insert(
          1,
          os.path.join(path_util.GetSrcDir(), 'testing', 'scripts',
                       'run_performance_tests.py'))

      args.append(f'--benchmarks={benchmark_name}')
      assert out_dir
      args.append('--isolated-script-test-output=' +
                  os.path.join(out_dir, benchmark_name, 'output.json'))

    if self.profile_dir:
      args.append(f'--profile-dir={self.profile_dir}')

    args.append('--browser=exact')
    args.append(f'--browser-executable={self.binary_path}')
    args.append('--pageset-repeat=%d' % benchmark_config.pageset_repeat)

    if len(benchmark_config.stories) > 0:
      for story in benchmark_config.stories:
        args.append(f'--story={story}')

    extra_browser_args = deepcopy(config.extra_browser_args)
    extra_browser_args.extend(config.browser_type.GetExtraBrowserArgs())
    if self.field_trial_config:
      extra_browser_args.append(
          f'--field-trial-config={self.field_trial_config}')

    args.extend(config.browser_type.GetExtraBenchmarkArgs())
    args.extend(config.extra_benchmark_args)

    if self.common_options.verbose:
      args.extend(['--show-stdout', '--verbose'])

    if len(extra_browser_args) > 0:
      args.append('--extra-browser-args=' + ' '.join(extra_browser_args))

    success, _ = perf_test_utils.GetProcessOutput(
        args, cwd=path_util.GetChromiumPerfDir())
    return success

  def TakeStatusLine(self):
    status_line = self.status_line
    self.status_line = ''
    return status_line

  def RunTests(self) -> bool:
    assert (self.binary_path is not None)
    has_failure = False

    if not self.PrepareProfile():
      return False

    start_time = time.time()
    for benchmark in self.benchmarks:
      if self.common_options.local_run:
        test_out_dir = os.path.join(self.out_dir, os.pardir, benchmark.name)
      else:
        test_out_dir = os.path.join(self.out_dir, 'results')
      logging.info('Running test %s', benchmark.name)

      test_success = self.RunSingleTest(self.config, benchmark, test_out_dir,
                                        self.common_options.local_run)

      if not test_success:
        has_failure = True
        error = f'Test {benchmark.name} failed on binary {self.binary_path}'
        error += '\nLogs: ' + os.path.join(test_out_dir, benchmark.name,
                                           benchmark.name, 'benchmark_log.txt')
        self.logs.append(error)

    spent_time = time.time() - start_time
    self.status_line += f'Run {spent_time:.2f}s '
    self.status_line += 'FAILURE  ' if has_failure else 'OK  '
    return not has_failure

  def ReportToDashboard(self) -> bool:
    logging.info('Reporting to dashboard %s...', self.config.label)
    start_time = time.time()
    assert (self.config.dashboard_bot_name is not None)
    assert (self.config.tag is not None)
    report_success, report_failed_logs, revision_number = ReportToDashboardImpl(
        self.config.browser_type, self.config.dashboard_bot_name,
        f'refs/tags/{self.config.tag}', os.path.join(self.out_dir, 'results'))
    spent_time = time.time() - start_time
    self.status_line += f'Report {spent_time:.2f}s '
    self.status_line += 'OK, ' if report_success else 'FAILURE, '
    self.status_line += f'Revnum: #{revision_number}'
    if not report_success:
      self.logs.extend(report_failed_logs)
    return report_success

  def ClearArtifacts(self):
    if self.common_options.local_run:
      for benchmark in self.benchmarks:
        artifacts_dir = os.path.join(self.out_dir, os.pardir, benchmark.name,
                                     'artifacts')
        shutil.rmtree(artifacts_dir)

  def Run(self) -> Tuple[bool, List[str]]:
    self.logs = []
    logging.info('##Label: %s binary %s', self.config.label, self.binary_path)
    run_tests_ok = True
    report_ok = True

    if self.common_options.do_run_tests:
      run_tests_ok = self.RunTests()
    if self.common_options.do_report:
      if run_tests_ok or self.common_options.report_on_failure:
        report_ok = self.ReportToDashboard()
    self.logs.append(self.TakeStatusLine())
    if run_tests_ok and report_ok and not self.config.save_artifacts:
      self.ClearArtifacts()

    return run_tests_ok and report_ok, self.logs


def PrepareBinariesAndDirectories(configurations: List[RunnerConfig],
                                  benchmarks: List[BenchmarkConfig],
                                  common_options: CommonOptions
                                  ) -> List[RunableConfiguration]:
  runable_configurations: List[RunableConfiguration] = []
  for config in configurations:
    if config.tag == config.label:
      description = str(config.tag)
    else:
      description = f'{config.label}'
      if config.tag is not None:
        description += f'[tag_{config.tag}]'
    assert (description)
    out_dir = os.path.join(common_options.working_directory, description)

    binary_path = None
    field_trial_config = None

    if common_options.do_run_tests:
      shutil.rmtree(out_dir, True)
      os.makedirs(out_dir)
      binary_path = PrepareBinary(out_dir, config.tag, config.location,
                                  config.browser_type)
      assert config.tag is not None
      field_trial_config = config.browser_type.MakeFieldTrials(
          config.tag, out_dir, common_options.variations_repo_dir)
      logging.info('%s : %s directory %s', description, binary_path, out_dir)
    runable_configurations.append(
        RunableConfiguration(config, benchmarks, binary_path, out_dir,
                             common_options, field_trial_config))
  return runable_configurations


def SpawnConfigurationsFromTargetList(target_list: List[str],
                                      base_configuration: RunnerConfig
                                      ) -> List[RunnerConfig]:
  configurations: List[RunnerConfig] = []
  for target_string in target_list:
    config = deepcopy(base_configuration)
    config.tag, location = ParseTarget(target_string)
    if not config.location:
      config.location = location
    if not config.tag:
      raise RuntimeError(f'Can get the tag from target {target_string}')
    config.label = str(config.tag)
    configurations.append(config)
  return configurations


def RunConfigurations(configurations: List[RunnerConfig],
                      benchmarks: List[BenchmarkConfig],
                      common_options: CommonOptions) -> bool:
  runable_configurations = PrepareBinariesAndDirectories(
      configurations, benchmarks, common_options)

  has_failure = False
  logs: List[str] = []
  for config in runable_configurations:
    result, config_logs = config.Run()
    if not result:
      has_failure = True
    logs.extend(config_logs)

  if common_options.local_run:
    for benchmark in benchmarks:
      logs.append(benchmark.name + ' : file://' + os.path.join(
          common_options.working_directory, benchmark.name, 'results.html'))

  if logs != []:
    logging.info('Logs:')
    for item in logs:
      logging.info(item)

  if has_failure:
    logging.error('Summary: not ok')
  else:
    logging.info('Summary: OK')

  return not has_failure
