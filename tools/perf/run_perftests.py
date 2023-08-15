#!/usr/bin/env vpython3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
r"""A tool run telemetry perftests and report the results to dashboard

Use npm run perf_tests to call this script.

The tool:
1. Downloads browser binaries.
2. Runs the set of telemetry perftests from the provided config.
3. Reports the results to brave-perf-dashboard.appspot.com or store them as
   local .html files.

The tool is best run on specially prepared hardware/OS to minimize jitter.

"""
import argparse
from codecs import ignore_errors
import logging
import sys
import os

import components.perf_config as perf_config
import components.perf_test_runner as perf_test_runner
import components.perf_test_utils as perf_test_utils


def main():
  parser = argparse.ArgumentParser(
      formatter_class=argparse.RawTextHelpFormatter,
      description='A tool to run perf tests and report the results.'
                  'Use it via npm run perf_tests',
      epilog=R'''
Example usage to launch tests locally:
npm run perf_tests -- compare\compare_with_on_off_feature.json5
     --variations-repo-dir=e:\work\brave-variations
     --local-run

Example usage for CI:
npm run perf_tests -- smoke-brave.json5 v1.58.45
     --working-directory=e:\work\brave\src\out\100
     --variations-repo-dir=e:\work\brave-variations
     --ci-mode
''')
  perf_test_runner.CommonOptions.add_parser_args(parser)

  args = parser.parse_args()
  options = perf_test_runner.CommonOptions.from_args(args)

  log_level = logging.DEBUG if options.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)

  logging.info('Use working directory %s', options.working_directory)
  if not os.path.exists(options.working_directory):
    os.mkdir(options.working_directory)
  json_config = perf_test_utils.LoadJsonConfig(args.config,
                                               options.working_directory)
  config = perf_config.PerfConfig(json_config)

  if options.compare:  # compare mode
    configurations = config.runners
  else:
    if len(config.runners) != 1:
      raise RuntimeError('Only one configuration should be specified.')

    configurations = perf_test_runner.SpawnConfigurationsFromTargetList(
        options.targets, config.runners[0])

  return 0 if perf_test_runner.RunConfigurations(
      configurations, config.benchmarks, options) else 1


if __name__ == '__main__':
  sys.exit(main())
