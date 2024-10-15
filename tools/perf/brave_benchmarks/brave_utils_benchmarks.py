# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# A special utility benchmark that doesn't measure anything.
#
# Used as part of chromium src/tools/perf/benchmarks/
# Uses the the same code conventions (including pylint).
# pylint: disable=import-error, no-self-use
# pylint: disable=no-name-in-module, too-few-public-methods
# pytype: disable=import-error

import os

from core import perf_benchmark

from core.path_util import SysPath, GetChromiumSrcDir

with SysPath(os.path.join(GetChromiumSrcDir(), 'brave', 'tools', 'perf')):
  from brave_page_sets.brave_perf_utils_pages import BravePerfUtilsStorySet


class BraveUtilsBenchmark(perf_benchmark.PerfBenchmark):
  """A special benchmark for utility purposes.

    Doesn't measure anything.
  """

  def CreateStorySet(self, options):
    return BravePerfUtilsStorySet(30, options)

  @classmethod
  def Name(cls):
    return 'brave_utils.offline'


class BraveUtilsOnlineBenchmark(BraveUtilsBenchmark):
  """A version of BraveUtilsBenchmark with an internet connection."""
  options = {
      'use_live_sites': True,
  }

  def CreateStorySet(self, options):
    return BravePerfUtilsStorySet(3 * 60, options)

  @classmethod
  def Name(cls):
    return 'brave_utils.online'
