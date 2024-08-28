# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# pylint: disable=too-many-instance-attributes

from enum import Enum

import argparse
import os
import sys
import tempfile
import platform
from typing import List, Optional

import components.path_util as path_util

# pylint: disable=import-error
# pytype: disable=import-error
with path_util.SysPath(path_util.GetTelemetryDir()):
  with path_util.SysPath(path_util.GetChromiumPerfDir()):
    from core.perf_benchmark import PerfBenchmark
# pylint: enable=import-error
# pytype: enable=import-error


class PerfMode(Enum):
  RUN = 1
  COMPARE = 2
  UPDATE_PROFILE = 3
  RECORD_WPR = 4

class CommonOptions:
  mode: PerfMode = PerfMode.RUN
  verbose: bool = False
  ci_mode: bool = False
  chromium: bool = False
  machine_id: Optional[str] = None
  variations_repo_dir: Optional[str] = None
  working_directory: str = ''
  target_os: str = PerfBenchmark.FixupTargetOS(sys.platform)
  target_arch: str = ''

  do_report: bool = False
  upload: bool = True
  upload_branch: Optional[str] = None
  report_on_failure: bool = False
  local_run: bool = False
  retry_count = 2
  targets: List[str] = []
  config: str = ''

  @property
  def is_android(self) -> bool:
    return self.target_os == 'android'

  @classmethod
  def add_parser_args(cls, parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        'config',
        type=str,
        help='The path/URL to a config. See configs/**/.json for examples.'
        'Also could be set to "auto" to select the config by '
        'machine-id + chromium')
    parser.add_argument(
        'targets',
        type=str,
        nargs='?',
        help='Format: version1[:<path_or_url1>],..,versionN[:<path_or_urlN>].'
        'Empty value enables the compare mode (see --compare).')
    parser.add_argument(
        '--mode',
        type=str,
        choices=['run', 'compare', 'update-profile', 'record-wpr'],
        help='The operating mode.' +
        '"run" is run the tests and report to the backend (the default).' +
        '"compare" is evaluate a few configurations with a local HTML output.' +
        '"update-profile" is a tool to update and upload profile archives.')
    parser.add_argument(
        '--working-directory',
        type=str,
        help='A main directory to store binaries, artifacts and results.'
        'Is equal to a temp directory by default.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose logging.')
    parser.add_argument(
        '--variations-repo-dir',
        type=str,
        help='A path to brave-variation repository to use Griffin in tests')
    parser.add_argument('--target-os',
                        '--target_os',
                        type=str,
                        choices=['windows', 'mac', 'linux', 'android'])
    parser.add_argument('--target-arch', type=str, choices=['x64', 'arm64'])
    parser.add_argument(
        '--local-run',
        action='store_true',
        help='Store results locally as html, don\'t report to the dashboard')

    parser.add_argument('--ci-mode',
                        action='store_true',
                        help='Used for CI (brave-browser-test-perf-* builds).')
    parser.add_argument('--chromium',
                        action='store_true',
                        help='(with config=auto) Run chromium (reference) build'
                        'Used select the config by machine-id + chromium')
    parser.add_argument('--machine-id',
                        type=str,
                        help='(with config=auto) The name of machine on CI.'
                        'Used select the config by machine-id + chromium')
    parser.add_argument('--retry-count',
                        type=int,
                        default=2,
                        help='Number of retries for a failed benchmark')
    parser.add_argument('--no-report',
                        action='store_true',
                        help='[ci-mode] Don\'t to the dashboard')
    parser.add_argument(
        '--report-on-failure',
        action='store_true',
        help='[ci-mode] Report to the dashboard despite test failures')
    parser.add_argument(
        '--upload',
        action='store_true',
        default=True,
        help=(
            '[For profile updating] Upload the updated profile to cloud storage'
            + 'and push the changes to brave-core'))
    parser.add_argument(
        '--upload-branch',
        type=str,
        help=('[For profile updating] A target brave-core branch to push the ' +
              'changes. update-profiles-<version> is used by default'))

    parser.add_argument('--more-help',
                        action='help',
                        help='Show this help message and exit.')

  @classmethod
  def from_args(cls, args) -> 'CommonOptions':
    options = CommonOptions()
    if args.working_directory is None:
      if options.ci_mode:
        raise RuntimeError('Set --working-directory for --ci-mode')
      options.working_directory = tempfile.mkdtemp(prefix='perf-test-')
    else:
      options.working_directory = os.path.expanduser(args.working_directory)

    empty_target = args.targets is None or args.targets == ''
    if args.mode == 'run':
      options.mode = PerfMode.RUN
    elif args.mode == 'compare' or (args.mode is None and empty_target):
      options.mode = PerfMode.COMPARE
    elif args.mode == 'update-profile':
      options.mode = PerfMode.UPDATE_PROFILE
    elif args.mode == 'record-wpr':
      options.mode = PerfMode.RECORD_WPR

    options.verbose = args.verbose
    options.ci_mode = args.ci_mode
    options.chromium = args.chromium
    options.machine_id = args.machine_id

    if args.variations_repo_dir is not None:
      options.variations_repo_dir = os.path.expanduser(args.variations_repo_dir)
    if args.target_os is not None:
      options.target_os = PerfBenchmark.FixupTargetOS(args.target_os)

    if args.target_arch is not None:
      options.target_arch = args.target_arch
    else:
      if options.target_os == 'android' or platform.processor() == 'arm':
        options.target_arch = 'arm64'
      else:
        options.target_arch = 'x64'

    options.report_on_failure = args.report_on_failure
    if args.targets is not None:
      options.targets = args.targets.split(',')

    options.local_run = args.local_run or options.mode != PerfMode.RUN
    if args.retry_count is not None:
      options.retry_count = args.retry_count

    options.do_report = (not args.no_report and not args.local_run
                         and options.mode == PerfMode.RUN)
    options.upload = args.upload
    options.upload_branch = args.upload_branch

    return options
