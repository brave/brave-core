# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
"""A inline part of loading.py"""

#pylint: disable=no-self-use,undefined-variable


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

    @classmethod
    def Name(cls):
        return 'loading.desktop.brave.startup'
