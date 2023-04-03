#!/usr/bin/env vpython3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
r"""A tool run telemetry perftests and report the results to dashboard


The tool:
1. Download a proper binary.
2. Run a subset of telemetry perftests from the provided config.
3. Report the result to brave-perf-dashboard.appspot.com
The tool is best run on specially prepared hardware/OS to minimize jitter.

Example usage:
 vpython3 run_dashboard_perftests.py --working-directory=e:\work\tmp\perf0\
                                     --config=smoke.json
                                     --target v1.36.23
"""
import sys
import logging
import argparse

import components.perf_test_utils as perf_test_utils
import components.perf_config as perf_config
import components.perf_test_runner as perf_test_runner


def main():
  parser = argparse.ArgumentParser()
  perf_test_runner.CommonOptions.add_common_parser_args(parser)
  parser.add_argument('--targets',
                      required=True,
                      type=str,
                      help='Tags/binaries to test')
  parser.add_argument('--config', required=True, type=str)
  parser.add_argument('--target_os', type=str)
  parser.add_argument('--no-report', action='store_true')
  parser.add_argument('--report-only', action='store_true')
  parser.add_argument('--report-on-failure', action='store_true')
  parser.add_argument('--local-run', action='store_true')

  args = parser.parse_args()

  log_level = logging.DEBUG if args.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)

  targets = args.targets.split(',')

  json_config = perf_test_utils.LoadJsonConfig(args.config)
  config = perf_config.PerfConfig(json_config)

  common_options = perf_test_runner.CommonOptions.from_args(args)

  common_options.do_run_tests = not args.report_only
  common_options.do_report = not args.no_report and not args.local_run
  common_options.report_on_failure = args.report_on_failure
  common_options.local_run = args.local_run

  if len(config.runners) != 1:
    print( config.runners)
    raise RuntimeError('Only one configuration should be specified.')

  configurations = perf_test_runner.SpawnConfigurationsFromTargetList(
      targets, config.runners[0])

  return 0 if perf_test_runner.RunConfigurations(
      configurations, config.benchmarks, common_options) else 1


if __name__ == '__main__':
  sys.exit(main())
