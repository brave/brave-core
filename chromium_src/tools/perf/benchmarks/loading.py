# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
"""A inline part of loading.py"""

#pylint: disable=no-self-use,undefined-variable


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
class LoadingDesktopBrave(_LoadingBase):
    """ A benchmark measuring loading performance of desktop sites. """
    SUPPORTED_PLATFORM_TAGS = [platforms.DESKTOP]
    SUPPORTED_PLATFORMS = [story.expectations.ALL_DESKTOP]

    def CreateStorySet(self, _options):
        return page_sets.BraveLoadingDesktopStorySet(
            cache_temperatures=[cache_temperature.COLD, cache_temperature.WARM])

    def CreateCoreTimelineBasedMeasurementOptions(self):
        return CreateCoreTBMOptions(['braveGeneralUmaMetric'])

    @classmethod
    def Name(cls):
        return 'loading.desktop.brave'


@benchmark.Info(emails=['matuchin@brave.com', 'iefremov@brave.com'],
                component='Blink>Loader',
                documentation_url='https://bit.ly/loading-benchmarks')
class LoadingDesktopBraveStartup(_LoadingBase):
    """ A benchmark measuring loading performance of desktop sites. """
    SUPPORTED_PLATFORM_TAGS = [platforms.DESKTOP]
    SUPPORTED_PLATFORMS = [story.expectations.ALL_DESKTOP]

    def CreateStorySet(self, _options):
        return page_sets.BraveLoadingDesktopStorySet(
            cache_temperatures=[cache_temperature.COLD, cache_temperature.WARM],
            with_delay=False)

    def CreateCoreTimelineBasedMeasurementOptions(self):
        return CreateCoreTBMOptions(
            ['braveGeneralUmaMetric', 'braveStartupUmaMetric'])

    @classmethod
    def Name(cls):
        return 'loading.desktop.brave.startup'
