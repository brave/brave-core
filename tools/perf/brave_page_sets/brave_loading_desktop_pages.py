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


class BaseBraveLoadingDesktopStorySet(story.StorySet):
  """ Base class for Brave Loading Desktop Story Sets. """

  def __init__(self, archive_file_name, startup_delay=False):
    archive_data_file = GetPageSetsDataPath(archive_file_name)
    super().__init__(archive_data_file=archive_data_file,
                     cloud_storage_bucket=story.PARTNER_BUCKET)
    self.startup_delay = startup_delay
    self.AddStories(['typical'],
                    [('https://example.com/', 'example.com'),
                     ('https://search.brave.com/', 'BraveSearch'),
                     ('https://en.wikipedia.org/wiki/HCard', 'wikipedia.com'),
                     ('https://www.economist.com/', 'Economist'),
                     ('https://www.ign.com/', 'IGN')])

  def AddStories(self, tags, urls):
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

        shared_page_state_class = (DelayedSharedDesktopPageState
                                   if self.startup_delay else
                                   shared_page_state.SharedDesktopPageState)

        self.AddStory(
            self._create_story(url,
                               self,
                               shared_page_state_class,
                               cache_temperature=temp,
                               tags=page_tags,
                               name=page_name,
                               perform_final_navigation=True))

  def _create_story(self, url, story_set, shared_page_state_class,
                    cache_temperature, tags, name, perform_final_navigation):
    """ Create a story instance. To be overridden in derived classes. """
    raise NotImplementedError


class BraveLoadingDesktopStorySet(BaseBraveLoadingDesktopStorySet):
  """ Brave version of LoadingDesktopStorySet with PageCyclerStory.

    See loading_desktop.py for details.
    """

  def __init__(self, startup_delay=False):
    super().__init__('brave_loading_desktop.json', startup_delay)

  def _create_story(self, url, story_set, shared_page_state_class,
                    cache_temperature, tags, name, perform_final_navigation):
    return page_cycler_story.PageCyclerStory(
        url,
        story_set,
        shared_page_state_class,
        cache_temperature=cache_temperature,
        tags=tags,
        name=name,
        perform_final_navigation=perform_final_navigation)


class BraveMultitabLoadingDesktopStorySet(BaseBraveLoadingDesktopStorySet):
  """ Brave version of LoadingDesktopStorySet with MultiTab story. """

  def __init__(self, startup_delay=False):
    super().__init__('brave_loading_desktop.json', startup_delay)

  class MultiTabStory(page_cycler_story.PageCyclerStory):
    """ A story that opens multiple tabs. """
    TAB_COUNT = 10

    def RunPageInteractions(self, action_runner):
      tabs = action_runner.tab.browser.tabs
      tabs[0].Navigate(self.url)
      for _ in range(1, self.TAB_COUNT):
        new_tab = tabs.New()
        new_tab.action_runner.Navigate(self.url)

  def _create_story(self, url, story_set, shared_page_state_class,
                    cache_temperature, tags, name, perform_final_navigation):
    return self.MultiTabStory(url,
                              story_set,
                              shared_page_state_class,
                              cache_temperature=cache_temperature,
                              tags=tags,
                              name=name,
                              perform_final_navigation=perform_final_navigation)
