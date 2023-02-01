#!/usr/bin/env vpython3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
r"""A tool to make a local comparison of few binaries/args

The tool:
1. Download proper binaries and profiles.
2. Run a subset of telemetry perftests from the provided config.
3. Store results as a local .html file.

The tool is best run on specially prepared hardware/OS to minimize jitter.
 vpython3 perf_compare.py --working-directory=e:\work\tmp\perf0\
                          --config=compare_configs.json
"""

import argparse
import sys
import logging

import components.perf_test_utils as perf_test_utils
import components.perf_test_runner as perf_test_runner
import components.perf_config as perf_config


def main():
  parser = argparse.ArgumentParser()
  perf_test_runner.CommonOptions.add_common_parser_args(parser)
  parser.add_argument('--config', required=True, type=str)
  args = parser.parse_args()

  log_level = logging.DEBUG if args.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)

  json_config = perf_test_utils.LoadJsonConfig(args.config)
  config = perf_config.PerfConfig(json_config)

  common_options = perf_test_runner.CommonOptions.from_args(args)
  common_options.local_run = True

  return 0 if perf_test_runner.RunConfigurations(
      config.runners, config.benchmarks, common_options) else 1


if __name__ == '__main__':
  sys.exit(main())
