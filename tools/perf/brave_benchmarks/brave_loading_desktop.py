# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# A brave version of loading.desktop benchmark

# Used as part of chromium src/tools/perf/benchmarks/
# Uses the the same code conventions (including pylint).
# pylint: disable=import-error, no-self-use
# pylint: disable=no-name-in-module, too-few-public-methods
# pytype: disable=import-error

import os
import time

from benchmarks import loading_metrics_category

from core import perf_benchmark
from core import platforms

from core.path_util import SysPath, GetChromiumSrcDir

from telemetry import story
from telemetry import benchmark

from telemetry.web_perf import timeline_based_measurement

with SysPath(os.path.join(GetChromiumSrcDir(), 'brave', 'tools', 'perf')):
  from brave_page_sets.brave_loading_desktop_pages import (
      BraveLoadingDesktopStorySet)


def CreateCoreTBMOptions(metric_list):
  tbm_options = timeline_based_measurement.Options()
  loading_metrics_category.AugmentOptionsForLoadingMetrics(tbm_options)
  cat_filter = tbm_options.config.chrome_trace_config.category_filter
  cat_filter.AddDisabledByDefault('disabled-by-default-histogram_samples')
  tbm_options.ExtendTimelineBasedMetric(metric_list)
  return tbm_options


@benchmark.Info(emails=['matuchin@brave.com', 'iefremov@brave.com'],
                component='Blink>Loader',
                documentation_url='https://bit.ly/loading-benchmarks')
class LoadingDesktopBrave(perf_benchmark.PerfBenchmark):
  """ A benchmark measuring loading performance of desktop sites. """
  SUPPORTED_PLATFORM_TAGS = [platforms.DESKTOP]
  SUPPORTED_PLATFORMS = [story.expectations.ALL_DESKTOP]

  def CreateStorySet(self, _options):
    return BraveLoadingDesktopStorySet(startup_delay=True)

  def CreateCoreTimelineBasedMeasurementOptions(self):
    return CreateCoreTBMOptions(
        ['braveGeneralUmaMetric', 'braveNavigationMetric'])

  def WillRunStory(self, _story):
    time.sleep(10)
    super().WillRunStory(self, _story)

  @classmethod
  def Name(cls):
    return 'loading.desktop.brave'


@benchmark.Info(emails=['matuchin@brave.com', 'iefremov@brave.com'],
                component='Blink>Loader',
                documentation_url='https://bit.ly/loading-benchmarks')
class LoadingDesktopBraveStartup(perf_benchmark.PerfBenchmark):
  """ A benchmark measuring loading performance of desktop sites. """
  SUPPORTED_PLATFORM_TAGS = [platforms.DESKTOP]
  SUPPORTED_PLATFORMS = [story.expectations.ALL_DESKTOP]

  def CreateStorySet(self, _options):
    return BraveLoadingDesktopStorySet(startup_delay=False)

  def CreateCoreTimelineBasedMeasurementOptions(self):
    return CreateCoreTBMOptions(
        ['braveGeneralUmaMetric', 'braveStartupUmaMetric'])

  @classmethod
  def Name(cls):
    return 'loading.desktop.brave.startup'
