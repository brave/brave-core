# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
"""A inline part of loading.py"""

from benchmarks import press


@benchmark.Info(emails=['matuchin@brave.com', 'iefremov@brave.com'],
                component='Blink>Loader',
                documentation_url='https://bit.ly/loading-benchmarks')
class LoadingDesktopBrave(LoadingDesktop):
    """ A benchmark measuring loading performance of desktop sites. """
    SUPPORTED_PLATFORM_TAGS = [platforms.DESKTOP]
    SUPPORTED_PLATFORMS = [story.expectations.ALL_DESKTOP]

    def CreateStorySet(self, options):
        return page_sets.BraveLoadingDesktopStorySet(
            cache_temperatures=[cache_temperature.COLD, cache_temperature.WARM])

    @classmethod
    def Name(cls):
        return 'loading.desktop.brave'


@benchmark.Info(emails=['matuchin@brave.com', 'iefremov@brave.com'],
                component='Brave>Memory')
class BinarySizeBrave(press._PressBenchmark):  # pylint: disable=protected-access
    """ A fake benchmark to report the binary size to the dashboard. """
    binary_size = 0

    def CreateStorySet(self, options):
        return page_sets.BinarySizeStorySet(self.binary_size)

    @classmethod
    def AddBenchmarkCommandLineArgs(cls, parser):
        parser.add_option('--binary-size',
                          type="int",
                          help="The binary size in bytes to be reported")

    @classmethod
    def ProcessCommandLineArgs(cls, parser, args):
        if args.binary_size is None:
            raise RuntimeError('--binary-size=<value> must be provided')
        cls.binary_size = args.binary_size

    @classmethod
    def Name(cls):
        return 'binary_size.brave'
