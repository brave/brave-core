# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# A page set for loading.desktop benchmark.brave benchmark.
#
# Used as part of chromium src/tools/perf/page_sets/
# Uses the the same code conventions (including pylint).
# pylint: disable=import-error, no-self-use
# pylint: disable=no-name-in-module, too-few-public-methods
# pytype: disable=import-error
import os
import time

from page_sets import page_cycler_story

from telemetry.page import cache_temperature as cache_temperature_module
from telemetry.page import shared_page_state
from telemetry import story

from core.path_util import SysPath, GetChromiumSrcDir

with SysPath(os.path.join(GetChromiumSrcDir(), 'brave', 'tools', 'perf')):
  from components.path_util import GetPageSetsDataPath


class DelayedSharedDesktopPageState(shared_page_state.SharedDesktopPageState):
  """A version of SharedDesktopPageState with 10 sec delay after the startup."""

  def _StartBrowser(self, page):
    super()._StartBrowser(page)
    time.sleep(10)


class BraveLoadingDesktopStorySet(story.StorySet):
  """ Brave version of LoadingDesktopStorySet.

  See loading_desktop.py for details.
  """

  def __init__(self, startup_delay=False):
    archive_data_file = GetPageSetsDataPath('brave_loading_desktop.json')
    super().__init__(archive_data_file=archive_data_file,
                         cloud_storage_bucket=story.PARTNER_BUCKET)

    # Passed as (story, name) tuple.
    self.AddStories(['typical'],
                    [('https://example.com/', 'example.com'),
                     ('https://search.brave.com/', 'BraveSearch'),
                     ('https://en.wikipedia.org/wiki/HCard', 'wikipedia.com'),
                     ('https://www.economist.com/', 'Economist'),
                     ('https://www.ign.com/', 'IGN')], startup_delay)

  def AddStories(self, tags, urls, startup_delay=False):
    cache_temperatures = [
        cache_temperature_module.COLD, cache_temperature_module.WARM
    ]
    for url, name in urls:
      for temp in cache_temperatures:
        if temp == cache_temperature_module.COLD:
          page_name = name + '_cold'
          tags.append('cache_temperature_cold')
        elif temp == cache_temperature_module.WARM:
          page_name = name + '_warm'
          tags.append('cache_temperature_warm')
        else:
          raise NotImplementedError

        page_tags = tags[:]

        if startup_delay:
          shared_page_state_class = DelayedSharedDesktopPageState
        else:
          shared_page_state_class = shared_page_state.SharedDesktopPageState

        self.AddStory(
            page_cycler_story.PageCyclerStory(url,
                                              self,
                                              shared_page_state_class,
                                              cache_temperature=temp,
                                              tags=page_tags,
                                              name=page_name,
                                              perform_final_navigation=True))
