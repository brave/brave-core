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
import components.cloud_storage as cloud_storage

from components.common_options import CommonOptions

# Brave and Chromium update related-hosts.
# Normally we don't need this in any WPR.
_UPDATE_HOSTS = [
    'brave-core-ext.s3.brave.com',
    'go-updater.brave.com',
    'componentupdater.brave.com',
    'optimizationguide-pa.googleapis.com',
    'safebrowsingohttpgateway.googleapis.com',
]

# Hosts are related to some browser features (like Rewards).
# Cutting it reduces memory and CPU usage, but makes the picture less
# representative. It makes sense to remove them some tests (i.e. jetstream)
# and leave in others (i.e. system_health).
_SERVICE_HOSTS = [
    'redirector.brave.com',
    'geo.ads.brave.com',
    'static.ads.brave.com',
    'rewards.brave.com',
    'api.rewards.brave.com',
    'grant.rewards.brave.com',
    'collector.bsg.brave.com',
    'star-randsrv.bsg.brave.com',
    'p3a-json.brave.com',
    'brave-today-cdn.brave.com',
    'update.googleapis.com',
    'content-autofill.googleapis.com',
]


def run_httparchive(args: List[str]) -> str:
  _, output = perf_test_utils.GetProcessOutput(
      ['go', 'run', os.path.join('src', 'httparchive.go'), *args],
      cwd=os.path.join(path_util.GetCatapultDir(), 'web_page_replay_go'),
      check=True)
  return output


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

  run_httparchive(args)

  # clean the source files:
  for file in files:
    if file != output_file:
      os.unlink(file)
    os.unlink(file + '.sha1')


def cleanup_archive(file: str, include_service_hosts: bool) -> None:
  file = os.path.abspath(file)

  # Remove the duplicates by making an empty .wprgo and merging the current file to it.
  tmp_file = file + '.empty'
  run_httparchive(['trim', '--invert-match', '--host', 'none', file, tmp_file])
  run_httparchive(['merge', tmp_file, file, file])
  os.unlink(tmp_file)

  # Filter requests by hosts:
  hosts = []
  hosts += _UPDATE_HOSTS
  if include_service_hosts:
    hosts += _SERVICE_HOSTS
  for host in hosts:
    logging.info('Removing %s', host)
    run_httparchive(['trim', '--host', host, file, file])

  # Remove Chromium https://accounts.google.com/ListAccounts requests:
  run_httparchive([
      'trim', '--full_path', '/ListAccounts', '--host', 'accounts.google.com',
      file, file
  ])

  # Recalculate .sha1 file.
  cloud_storage.UpdateSha1(file)


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
  cleanup_archive(output_file, False)
  run_httparchive(['ls', output_file])

  # Copy the final .wprgo to the artifacts directory.
  artifacts_dir = os.path.join(options.working_directory, 'artifacts')
  os.makedirs(artifacts_dir, exist_ok=True)
  shutil.copy(output_file, artifacts_dir)

  logging.info('Output file to upload: %s', output_file)

  return True
