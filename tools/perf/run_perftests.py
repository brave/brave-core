#!/usr/bin/env vpython3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
r"""A tool run telemetry perftests and report the results to dashboard

Use npm run perf_tests to call this script.

The tool:
1. Downloads browser binaries.
2. Runs the set of telemetry perftests from the provided config.
3. Reports the results to brave-perf-dashboard.appspot.com or stores them as
   local .html files.

The tool gives more stable results on prepared hardware/OS to minimize jitter.

"""
import argparse
import logging
import shutil
import sys
import os
import tempfile

import components.perf_config as perf_config
import components.perf_test_runner as perf_test_runner
import components.perf_test_utils as perf_test_utils
import components.path_util as path_util
import components.profile_tools as profile_tools
import components.wpr_tools as wpr_tools

from components.common_options import CommonOptions, PerfMode


with path_util.SysPath(path_util.GetPyJson5Dir()):
  # pylint: disable=import-error # pytype: disable=import-error
  import json5
  # pylint: enable=import-error # pytype: enable=import-error


def load_config(config: str, options: CommonOptions) -> dict:
  if config.startswith('https://'):  # URL to download the config
    _, config_path = tempfile.mkstemp(dir=options.working_directory,
                                      prefix='config-')
    perf_test_utils.DownloadFile(config, config_path)

  elif os.path.isfile(config):  # Full config path
    config_path = config
  elif config == 'auto':  # Select the config by machine_id and chromium
    if options.machine_id is None:
      raise RuntimeError('Set --machine-id to use config=auto')

    prefix = 'chromium' if options.chromium else 'brave'
    config = (f'{prefix}-{options.target_os}-' +
              f'{options.target_arch}-{options.machine_id}.json5')
    logging.info('Using %s as config=auto', config)
    config_path = os.path.join(path_util.GetBravePerfConfigDir(), 'ci', config)

    if not os.path.isfile(config_path):
      raise RuntimeError(f'No config file {config_path}')
  else:  # config is a relative path
    config_path = os.path.join(path_util.GetBravePerfConfigDir(), config)
    if not os.path.isfile(config_path):
      raise RuntimeError(f'Can\'t find matching config {config}')

  with open(config_path, 'r', encoding='utf-8') as config_file:
    return json5.load(config_file)


def _RunUpdateProfile(config: perf_config.PerfConfig,
                      options: CommonOptions) -> int:
  if len(config.runners) != 1:
    raise RuntimeError('Only one configuration should be specified.')
  options.do_report = False
  config.runners[0].profile_rebase = perf_config.ProfileRebaseType.NONE
  config.benchmarks = [
      perf_config.BenchmarkConfig({
          'name': 'brave_utils.online',
          'pageset-repeat': 5,
          'stories': ['UpdateProfile']
      })
  ]

  configurations = perf_test_runner.SpawnConfigurationsFromTargetList(
      options.targets, config.runners[0])
  assert len(configurations) == 1
  profile_tools.CleanupBrowserComponents(configurations[0], options)
  if not perf_test_runner.RunConfigurations(configurations, config.benchmarks,
                                            options):
    return 1

  profile_tools.MakeUpdatedProfileArchive(configurations[0], options)
  return 0

def main():
  parser = argparse.ArgumentParser(
      formatter_class=argparse.RawTextHelpFormatter,
      description='A tool to run perf tests and report the results.'
      'Use npm run perf_tests to launch it',
      epilog=R'''
To some launch tests locally:
npm run perf_tests -- compare/compare_with_on_off_feature.json5

On CI:
npm run perf_tests -- smoke-brave.json5 v1.58.45
     --working-directory=e:\work\brave\src\out\100
     --ci-mode
''')
  CommonOptions.add_parser_args(parser)

  args = parser.parse_args()
  options = CommonOptions.from_args(args)

  log_level = logging.DEBUG if options.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)

  logging.info('Use working directory %s', options.working_directory)
  if options.ci_mode and os.path.exists(options.working_directory):
    logging.info('Cleaning working directory %s', options.working_directory)
    shutil.rmtree(options.working_directory)

  os.makedirs(options.working_directory, exist_ok=True)

  json_config = load_config(args.config, options)
  config = perf_config.PerfConfig(json_config)

  if options.mode == PerfMode.RUN:
    if len(config.runners) != 1:
      raise RuntimeError('Only one configuration should be specified.')

    configurations = perf_test_runner.SpawnConfigurationsFromTargetList(
        options.targets, config.runners[0])
    return 0 if perf_test_runner.RunConfigurations(
        configurations, config.benchmarks, options) else 1

  if options.mode == PerfMode.COMPARE:
    return 0 if perf_test_runner.RunConfigurations(
        config.runners, config.benchmarks, options) else 1

  if options.mode == PerfMode.UPDATE_PROFILE:
    return _RunUpdateProfile(config, options)

  if options.mode == PerfMode.RECORD_WPR:
    return 0 if wpr_tools.record_wpr(config, options) else 1

  raise RuntimeError('Unknown mode')

if __name__ == '__main__':
  sys.exit(main())
