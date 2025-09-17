# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
import argparse

from run_perftests import load_config
from components.common_options import CommonOptions, PerfMode


def _parse_cmd_args(args) -> CommonOptions:
  parser = argparse.ArgumentParser()
  CommonOptions.add_parser_args(parser)
  return CommonOptions.from_args(parser.parse_args(args))


_CI_ARGS = [
    '--ci-mode', '--working-directory', 'fake-dir', '--target_os', 'mac',
    '--machine-id', 'mac-mini1-sf', '--target_arch', 'arm64'
]


class TestCommonOptions(unittest.TestCase):

  def test_ci(self):
    args = ['auto', 'v1.87.1', *_CI_ARGS]
    options = _parse_cmd_args(args)
    self.assertEqual(options.mode, PerfMode.RUN)
    config = load_config(options)
    self.assertEqual(config['configurations'][0]['profile'],
                     'brave-typical-mac')

  def test_ci_chromium(self):
    args = ['auto', 'v1.87.1', '--chromium', *_CI_ARGS]
    options = _parse_cmd_args(args)
    self.assertEqual(options.mode, PerfMode.RUN)
    config = load_config(options)
    self.assertEqual(config['configurations'][0]['profile'],
                     'chrome-typical-mac')

  def test_update_profile(self):
    args = [
        'auto', 'v1.87.1', '--mode', 'update-profile', '--upload', 'true',
        '--upload-branch', 'fake-branch', *_CI_ARGS
    ]
    options = _parse_cmd_args(args)
    self.assertEqual(options.mode, PerfMode.UPDATE_PROFILE)
    self.assertEqual(options.machine_id, 'mac-mini1-sf')
    self.assertEqual(options.chromium, False)
    self.assertEqual(options.upload, True)

    self.assertTrue(
        options.config.endswith('brave-mac-arm64-mac-mini1-sf.json5'))
    self.assertTrue(load_config(options))

  def test_smoke(self):
    args = ['smoke', 'Static', *_CI_ARGS]
    options = _parse_cmd_args(args)
    self.assertEqual(options.mode, PerfMode.RUN)
    self.assertEqual(options.chromium, False)
    self.assertEqual(options.upload, True)

    self.assertTrue(options.config.endswith('smoke.json5'))
    self.assertTrue(load_config(options))

  def test_compare(self):
    args = ['compare.json5', '--ci-mode', '--working-directory', 'fake-dir']
    options = _parse_cmd_args(args)
    self.assertEqual(options.mode, PerfMode.COMPARE)
