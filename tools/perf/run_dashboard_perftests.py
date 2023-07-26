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
  perf_test_runner.CommonOptions.add_parser_args(parser)

  args = parser.parse_args()
  options = perf_test_runner.CommonOptions.from_args(args)

  log_level = logging.DEBUG if options.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)



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
