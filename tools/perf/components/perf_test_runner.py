# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at https://mozilla.org/MPL/2.0/.
import json
import logging
import os
import shutil
import time
import sys
from copy import deepcopy
from typing import List, Optional, Tuple, Set
from lib.util import scoped_cwd

import components.path_util as path_util
import components.perf_test_utils as perf_test_utils
from components.common_options import CommonOptions
from components.browser_binary_fetcher import BrowserBinary, PrepareBinary
from components.browser_type import BraveVersion
from components.perf_config import (BenchmarkConfig, ParseTarget,
                                    ProfileRebaseType, RunnerConfig)


def ReportToDashboardImpl(
    dashboard_bot_name: str, version: BraveVersion, griffin_rev: Optional[str],
    output_dir: str) -> Tuple[bool, List[str], Optional[str]]:

  args = [
      sys.executable,
      os.path.join(path_util.GetChromiumPerfDir(), 'process_perf_results.py')
  ]
  args.append(f'--configuration-name={dashboard_bot_name}')
  args.append(f'--task-output-dir={output_dir}')
  args.append('--output-json=' + os.path.join(output_dir, 'results.json'))

  logging.debug('Got revision %s', version.revision_number)

  build_properties = {}
  build_properties['bot_id'] = 'test_bot'
  build_properties['perf_dashboard_machine_group'] = 'BravePerf'

  build_properties['recipe'] = 'chromium'
  build_properties['slavename'] = 'test_bot'

  # Jenkins CI specific variable. Must be set when reporting is on.
  job_name = os.environ.get('JOB_NAME')
  build_number = os.environ.get('BUILD_NUMBER')
  if job_name is None or build_number is None:
    raise RuntimeError('Set env JOB_NAME & BUILD_NUMBER or use --local-run')
  build_properties['buildername'] = job_name
  build_properties['buildnumber'] = build_number

  chromium_version_str = version.chromium_version.to_string()
  os.environ['DASHBOARD_EXTRA_DIAG_brave_chrome_version'] = chromium_version_str
  os.environ['DASHBOARD_EXTRA_DIAG_brave_tag'] = version.to_string()
  os.environ['DASHBOARD_EXTRA_DIAG_brave_job_name'] = job_name
  os.environ['DASHBOARD_EXTRA_DIAG_brave_job_id'] = build_number

  if griffin_rev is not None:
    os.environ['DASHBOARD_EXTRA_DIAG_brave_variations_revisions'] = griffin_rev

  build_properties[
      'got_revision_cp'] = 'refs/heads/main@{#%s}' % version.revision_number
  build_properties['got_revision'] = version.git_revision

  # It's necessary for process_perf_results.py, will be removed in patched
  # MakeHistogramSetWithDiagnostics
  build_properties['got_v8_revision'] = '0'
  build_properties['got_webrtc_revision'] = '0'

  build_properties_serialized = json.dumps(build_properties)
  args.append('--build-properties=' + build_properties_serialized)

  success, _ = perf_test_utils.GetProcessOutput(args)
  if success:
    return True, [], version.revision_number

  return False, ['Reporting ' + version.revision_number + ' failed'], None


# pylint: disable=too-many-instance-attributes
class RunableConfiguration:
  common_options: CommonOptions
  benchmarks: List[BenchmarkConfig]
  config: RunnerConfig
  binary: BrowserBinary
  out_dir: str

  status_line: str = ''
  logs: List[str] = []

  def __init__(self, config: RunnerConfig, benchmarks: List[BenchmarkConfig],
               binary: BrowserBinary, out_dir: str,
               common_options: CommonOptions):
    self.config = config
    self.benchmarks = benchmarks
    self.binary = binary
    self.out_dir = out_dir
    self.common_options = common_options
    self.custom_perf_handlers = {'apk_size': self.RunApkSize}

  def Install(self):
    if self.common_options.is_android and self.binary.binary_path is not None:
      if self.config.version is not None:
        expected_version = None
        if self.config.browser_type.is_brave:
          expected_version = self.config.version.last_tag[1:]
        else:
          expected_version = self.config.version.chromium_version.to_string()
        self.binary.install_apk(expected_version)

  def RebaseProfile(self) -> bool:
    if (self.binary.profile_dir is None
        or self.config.profile_rebase == ProfileRebaseType.NONE):
      return True
    start_time = time.time()
    logging.info('Rebasing dir %s using binary %s', self.binary.profile_dir,
                 self.binary)
    rebase_runner_config = deepcopy(self.config)

    online_rebase = self.config.profile_rebase == ProfileRebaseType.ONLINE
    rebase_benchmark = BenchmarkConfig()
    rebase_benchmark.name = ('brave_utils.online'
                             if online_rebase else 'brave_utils.offline')
    rebase_benchmark.stories = ['UpdateProfile']
    rebase_benchmark.stories_exclude = []

    rebase_benchmark.pageset_repeat = 2 if online_rebase else 1

    REBASE_TIMEOUT = 240 * rebase_benchmark.pageset_repeat

    rebase_out_dir = os.path.join(self.out_dir, 'rebase_artifacts')
    result = self.RunSingleTest(rebase_runner_config, rebase_benchmark,
                                rebase_out_dir, True, REBASE_TIMEOUT)
    self.status_line += f'Rebase {(time.time() - start_time):.2f}s '
    return result

  def RunApkSize(self, out_dir: str):
    assert self.binary.binary_path is not None
    assert out_dir is not None
    os.makedirs(out_dir, exist_ok=True)
    args = [
        sys.executable,
        os.path.join(path_util.GetSrcDir(), 'build', 'android',
                     'resource_sizes.py'), '--output-format=histograms',
        '--output-dir', out_dir, self.binary.binary_path
    ]
    success, _ = perf_test_utils.GetProcessOutput(args, timeout=120)
    if success:
      with scoped_cwd(out_dir):
        shutil.move('results-chart.json', 'test_results.json')

    return success

  def MakeRunBenchmarkArgs(self,
                           benchmark_config: BenchmarkConfig) -> List[str]:
    browser_args = []
    binary = self.binary

    # add benchmark and story info:
    args = [benchmark_config.name]
    args.append('--pageset-repeat=%d' % benchmark_config.pageset_repeat)
    if len(benchmark_config.stories) > 0:
      story_filter = '|'.join(benchmark_config.stories)
      args.append(f'--story-filter=({story_filter})')

    if len(benchmark_config.stories_exclude) > 0:
      story_filter_exclude = '|'.join(benchmark_config.stories_exclude)
      args.append(f'--story-filter-exclude=({story_filter_exclude})')

    # process the binary-specific args:
    args.extend(binary.get_run_benchmark_args())
    browser_args.extend(self.binary.get_browser_args())

    # process the extra args from json config:
    args.extend(self.config.extra_benchmark_args)
    browser_args.extend(self.config.extra_browser_args)

    # Wrap browser_args:
    if len(browser_args) > 0:
      args.append('--extra-browser-args=' + ' '.join(browser_args))

    return args

  def RunSingleTest(self,
                    config: RunnerConfig,
                    benchmark_config: BenchmarkConfig,
                    out_dir: str,
                    local_run: bool,
                    timeout: Optional[int] = None) -> bool:
    args = [sys.executable]
    args.append(os.path.join(path_util.GetChromiumPerfDir(), 'run_benchmark'))

    benchmark_name = benchmark_config.name
    bench_out_dir = None
    args.extend(self.MakeRunBenchmarkArgs(benchmark_config))

    if local_run:
      assert config.label is not None
      args.append('--results-label=' + config.label)
      args.append(f'--output-dir={out_dir}')

    else:
      # .reference suffix for benchmark folder is used in
      # process_perf_results.py to report the data as reference.
      suffix = '.reference' if config.browser_type.report_as_reference else ''
      bench_out_dir = os.path.join(out_dir, benchmark_name,
                                   benchmark_name + suffix)
      if os.path.exists(bench_out_dir):
        shutil.rmtree(bench_out_dir)

      args.extend([
          f'--output-dir={bench_out_dir}', '--output-format=json-test-results',
          '--output-format=histograms'
      ])

    custom_handler = self.custom_perf_handlers.get(benchmark_name)
    if custom_handler is not None and not local_run:
      assert bench_out_dir
      return custom_handler(bench_out_dir)

    # Optimize redownloading trace_processor_shell: if the file exists use it.
    is_win = sys.platform == 'win32'
    trace_processor_path = os.path.join(
        path_util.GetChromiumPerfDir(), 'core', 'perfetto_binary_roller', 'bin',
        'trace_processor_shell' + ('.exe' if is_win else ''))
    if os.path.isfile(trace_processor_path):
      args.append(f'--trace-processor-path={trace_processor_path}')

    if self.common_options.verbose:
      args.extend(['--show-stdout', '--verbose'])

    success, _ = perf_test_utils.GetProcessOutput(
        args, cwd=path_util.GetChromiumPerfDir(), timeout=timeout)
    if success and not local_run:
      assert (out_dir is not None)
      assert (bench_out_dir is not None)

      # Make the same directory/file structure as used by Chromium
      # run_performance_tests.py.
      # This allows to call process_perf_results.py to report to the dashboard.
      with scoped_cwd(bench_out_dir):
        shutil.move('test-results.json', 'test_results.json')
        shutil.move('histograms.json', 'perf_results.json')

    return success

  def TakeStatusLine(self):
    status_line = self.status_line
    self.status_line = ''
    return status_line

  def RunTests(self) -> bool:
    has_failure = False

    if not self.RebaseProfile():
      return False

    start_time = time.time()
    for benchmark in self.benchmarks:
      if self.common_options.local_run:
        test_out_dir = os.path.join(self.out_dir, os.pardir, benchmark.name)
      else:
        test_out_dir = os.path.join(self.out_dir, 'results')
      logging.info('Running test %s', benchmark.name)

      test_success: Optional[bool] = None
      attempt: int = 0
      while test_success != True and attempt <= self.common_options.retry_count:
        attempt += 1
        test_success = self.RunSingleTest(self.config, benchmark, test_out_dir,
                                          self.common_options.local_run)
        if not test_success:
          error = (f'[attempt {attempt}] Test {benchmark.name}' +
                   f' failed on binary {self.binary}')
          self.logs.append(error)
      if not test_success:
        has_failure = True

    spent_time = time.time() - start_time
    self.status_line += f'Run {spent_time:.2f}s '
    self.status_line += 'FAILURE  ' if has_failure else 'OK  '
    return not has_failure

  def ReportToDashboard(self) -> bool:
    logging.info('Reporting to dashboard %s...', self.config.label)
    start_time = time.time()
    assert self.config.dashboard_bot_name is not None
    assert self.config.version is not None
    griffin_rev = None
    if self.binary.field_trial_config is not None:
      griffin_rev = self.binary.field_trial_config.revision

    report_success, report_failed_logs, revision_number = ReportToDashboardImpl(
        self.config.dashboard_bot_name, self.config.version, griffin_rev,
        os.path.join(self.out_dir, 'results'))
    spent_time = time.time() - start_time
    self.status_line += f'Report {spent_time:.2f}s '
    self.status_line += 'OK, ' if report_success else 'FAILURE, '
    self.status_line += f'Revnum: #{revision_number}'
    if not report_success:
      self.logs.extend(report_failed_logs)
    return report_success

  def ClearTelemetryArtifacts(self):
    if self.common_options.local_run:
      for benchmark in self.benchmarks:
        artifacts_dir = os.path.join(self.out_dir, os.pardir, benchmark.name,
                                     'artifacts')
        shutil.rmtree(artifacts_dir)

  def Run(self) -> Tuple[bool, List[str]]:
    self.logs = []
    logging.info('##Label: %s binary %s', self.config.label, self.binary)
    run_tests_ok = True
    report_ok = True

    self.Install()

    run_tests_ok = self.RunTests()
    if self.common_options.do_report:
      if run_tests_ok or self.common_options.report_on_failure:
        report_ok = self.ReportToDashboard()
    self.logs.append(self.TakeStatusLine())
    if run_tests_ok and report_ok and not self.config.save_artifacts:
      self.ClearTelemetryArtifacts()

    return run_tests_ok and report_ok, self.logs


def PrepareBinariesAndDirectories(configurations: List[RunnerConfig],
                                  benchmarks: List[BenchmarkConfig],
                                  common_options: CommonOptions
                                  ) -> List[RunableConfiguration]:
  runable_configurations: List[RunableConfiguration] = []
  all_labels: Set[str] = set()
  for config in configurations:
    assert config.label
    if config.label in all_labels:
      raise RuntimeError(f'Duplicated label {config.label}')
    all_labels.add(config.label)

    binary_dir = os.path.join(common_options.working_directory, 'binaries',
                              config.label)
    artifacts_dir = os.path.join(common_options.working_directory, 'artifacts',
                                 config.label)

    shutil.rmtree(binary_dir, True)
    shutil.rmtree(artifacts_dir, True)
    os.makedirs(binary_dir)
    os.makedirs(artifacts_dir)
    binary = PrepareBinary(binary_dir, artifacts_dir, config, common_options)
    logging.info('%s binary: %s artifacts: %s', config.label, binary,
                 artifacts_dir)
    runable_configurations.append(
        RunableConfiguration(config, benchmarks, binary, artifacts_dir,
                             common_options))
  return runable_configurations


def SpawnConfigurationsFromTargetList(target_list: List[str],
                                      base_configuration: RunnerConfig
                                      ) -> List[RunnerConfig]:
  configurations: List[RunnerConfig] = []
  for target_string in target_list:
    config = deepcopy(base_configuration)
    config.version, location = ParseTarget(target_string)
    if not config.location:
      config.location = location
    if not config.version:
      raise RuntimeError(f'Can get the version from target {target_string}')
    config.label = config.version.to_string()
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
      logs.append(benchmark.name + ' : file://' +
                  os.path.join(common_options.working_directory, 'artifacts',
                               benchmark.name, 'results.html'))

  if logs != []:
    logging.info('Logs:')
    for item in logs:
      logging.info(item)

  if has_failure:
    logging.error('Summary: not ok')
  else:
    logging.info('Summary: OK')

  return not has_failure
