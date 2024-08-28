# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import glob
import logging
import os
import shutil
import sys

from typing import List

import components.perf_config as perf_config
import components.perf_test_runner as perf_test_runner
import components.perf_test_utils as perf_test_utils
import components.path_util as path_util

from components.common_options import CommonOptions

_HOSTS_TO_REMOVE = [
    'brave-core-ext.s3.brave.com',  # components downloading
    'go-updater.brave.com',  # components update check
    'redirector.brave.com',
    'optimizationguide-pa.googleapis.com',  # optimizationguide chrome component
    'safebrowsingohttpgateway.googleapis.com',  # safebrowsing update
]


def _run_httparchive(args: List[str]) -> None:
  perf_test_utils.GetProcessOutput(
      ['go', 'run', os.path.join('src', 'httparchive.go'), *args],
      cwd=os.path.join(path_util.GetCatapultDir(), 'web_page_replay_go'),
      check=True)


def _get_wpr_pattern() -> str:
  return path_util.GetPageSetsDataPath('*.wprgo')


def _get_all_wpr_files() -> List[str]:
  files = []
  for file in glob.glob(_get_wpr_pattern()):
    files.append(file)
  return files


def _clean_all_wpr_files() -> None:
  for file in _get_all_wpr_files():
    os.unlink(file)


def _merge_wpr_files(files: List[str], output_file: str) -> None:
  if len(files) == 0:
    raise RuntimeError('No wprgo files to merge')

  args = ['merge']
  args.extend(files)
  args.append(output_file)

  _run_httparchive(args)

  # clean the source files:
  for file in files:
    if file != output_file:
      os.unlink(file)
    os.unlink(file + '.sha1')


def _post_process_wpr(file: str) -> None:
  for host in _HOSTS_TO_REMOVE:
    _run_httparchive(['trim', '--host', host, file, file])

  _run_httparchive(['ls', file])


def record_wpr(config: perf_config.PerfConfig, options: CommonOptions) -> bool:
  if len(config.runners) != 2:
    logging.warn('Normally you need specify two runners to record wpr: ' +
                 'for Brave and Chromium')
  options.do_report = False
  runable_configurations = perf_test_runner.PrepareBinariesAndDirectories(
      config.runners, config.benchmarks, options)

  _clean_all_wpr_files()

  for c in runable_configurations:
    c.Install()
    c.RebaseProfile()

    for benchmark in c.benchmarks:
      args = [sys.executable]
      args.append(os.path.join(path_util.GetChromiumPerfDir(), 'record_wpr'))
      args.extend(c.MakeRunBenchmarkArgs(benchmark))
      perf_test_utils.GetProcessOutput(args,
                                       cwd=path_util.GetChromiumPerfDir(),
                                       timeout=30 * 60,
                                       check=True)
  files = _get_all_wpr_files()
  output_file = max(files, key=os.path.getctime)
  _merge_wpr_files(files, output_file)
  _post_process_wpr(output_file)

  # Copy the final .wprgo to the artifacts directory.
  artifacts_dir = os.path.join(options.working_directory, 'artifacts')
  os.makedirs(artifacts_dir, exist_ok=True)
  shutil.copy(output_file, artifacts_dir)

  logging.info('Output file to upload: %s', output_file)

  return True
