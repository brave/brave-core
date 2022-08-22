# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

# Used as part of chromium src/tools/perf/page_sets/
# Therefore has the same code conventions (including pylint).
# pylint: disable=line-too-long, import-error, super-with-arguments
# pylint: disable=no-name-in-module, too-few-public-methods

import collections
import time

from page_sets import page_cycler_story
from telemetry.page import cache_temperature as cache_temperature_module
from telemetry.page import shared_page_state
from telemetry import story

Tag = collections.namedtuple('Tag', ['name', 'description'])


class DelayedSharedDesktopPageState(shared_page_state.SharedDesktopPageState):
  """ Brave version of SharedDesktopPageState with extras.

  1. A delay after the browser startup before running a test.
  2. It closes extra tabs after finihsing the test to reuse the profile later
     (used for rebasing profiles).
  See shared_page_state.py for details.
  """

  def _StartBrowser(self, page):
    super(shared_page_state.SharedDesktopPageState, self)._StartBrowser(page)
    # Wait a fixed time to finish all startup work.
    time.sleep(10)

  def _StopBrowser(self):
    if self._browser:
      # Try to close all previous tabs to maintain some independence between
      # individual story runs. Note that the final tab.Close(keep_one=True)
      # will create a fresh new tab before the last one is closed.
      while len(self._browser.tabs) > 1:
        self._browser.tabs[-1].Close()
      self._browser.tabs[-1].Navigate('chrome://newtab')

    super(shared_page_state.SharedDesktopPageState, self)._StopBrowser()


class BraveLoadingDesktopStorySet(story.StorySet):
  """ Brave version of LoadingDesktopStorySet.

  See loading_desktop.py for details.
  """

  def __init__(self, cache_temperatures=None):
    super(BraveLoadingDesktopStorySet,
          self).__init__(archive_data_file='data/brave_loading_desktop.json',
                         cloud_storage_bucket=story.PARTNER_BUCKET)

    if cache_temperatures is None:
      cache_temperatures = [
          cache_temperature_module.COLD, cache_temperature_module.WARM
      ]
    # Passed as (story, name) tuple.
    self.AddStories(['typical'],
                    [('https://example.com/', 'example.com'),
                     ('https://search.brave.com/', 'BraveSearch'),
                     ('https://en.wikipedia.org/wiki/HCard', 'wikipedia.com'),
                     ('https://www.economist.com/', 'Economist'),
                     ('https://www.ign.com/', 'IGN')], cache_temperatures)

  def AddStories(self, tags, urls, cache_temperatures):
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

        self.AddStory(
            page_cycler_story.PageCyclerStory(
                url,
                self,
                shared_page_state_class=DelayedSharedDesktopPageState,
                cache_temperature=temp,
                tags=page_tags,
                name=page_name,
                perform_final_navigation=True))
