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

from page_sets.system_health import story_tags
from page_sets.system_health import system_health_story
from page_sets.helpers import override_online

from telemetry.util import wpr_modes

from py_utils import TimeoutException


class _BraveLoadingStory(system_health_story.SystemHealthStory):
  """Abstract base class for single-page System Health user stories."""
  ABSTRACT_STORY = True

  SCROLL_PAGE = True

  def __init__(self, story_set, take_memory_measurement):
    super().__init__(story_set, take_memory_measurement)
    self.script_to_evaluate_on_commit = override_online.ALWAYS_ONLINE

  def _DidLoadDocument(self, action_runner):
    if self.wpr_mode == wpr_modes.WPR_RECORD:
      if self.SCROLL_PAGE:
        # Extra scroll to make sure that all resources are loaded:
        action_runner.RepeatableBrowserDrivenScroll(repeat_count=10)

      action_runner.Wait(10)
      return

    if self.SCROLL_PAGE:
      action_runner.RepeatableBrowserDrivenScroll(repeat_count=4)

  @classmethod
  def GenerateStoryDescription(cls):
    return 'Load %s' % cls.URL


class LoadExampleStory2023(_BraveLoadingStory):
  NAME = 'load:site:example:2023'
  URL = 'https://example.com'
  SCROLL_PAGE = False
  TAGS = [story_tags.YEAR_2023]


class LoadBraveSearchStory2023(_BraveLoadingStory):
  NAME = 'load:site:brave_search:2023'
  URL = 'https://search.brave.com/search?q=cats&source=web'
  TAGS = [story_tags.YEAR_2023]


class LoadGoogleStory2023(_BraveLoadingStory):
  NAME = 'load:site:google:2023'
  URL = 'https://www.google.com/search?q=cats'
  TAGS = [story_tags.YEAR_2023]


class LoadYoutubeStory2023(_BraveLoadingStory):
  NAME = 'load:site:youtube:2023'
  URL = 'https://www.youtube.com/watch?v=Way9Dexny3w'
  SCROLL_PAGE = False
  TAGS = [story_tags.YEAR_2023]


class LoadWikipediaStory2023(_BraveLoadingStory):
  NAME = 'load:site:wikipedia:2023'
  URL = 'https://en.wikipedia.org/wiki/World_Wide_Web'
  TAGS = [story_tags.YEAR_2023]

  def _DidLoadDocument(self, action_runner):
    try:
      action_runner.WaitForElement(selector='.js-close', timeout_in_seconds=5)
      action_runner.ClickElement(selector='.js-close')
    except TimeoutException:
      # Sometimes the pop up doesn't appear.
      pass
    super()._DidLoadDocument(action_runner)


class LoadTwitterStory2024(_BraveLoadingStory):
  NAME = 'load:site:twitter:2024'
  URL = 'https://x.com/SpaceX'
  TAGS = [story_tags.YEAR_2024]


class LoadCNNStory2023(_BraveLoadingStory):
  NAME = 'load:site:cnn:2023'
  URL = 'https://edition.cnn.com/'
  TAGS = [story_tags.YEAR_2023]


class LoadBBCStory2023(_BraveLoadingStory):
  NAME = 'load:site:bbc:2023'
  URL = 'https://www.bbc.com/'
  TAGS = [story_tags.YEAR_2023]


class LoadHackernewsStory2024(_BraveLoadingStory):
  NAME = 'load:site:hackernews:2024'
  URL = 'https://news.ycombinator.com/'
  SCROLL_PAGE = False
  TAGS = [story_tags.YEAR_2024]


class LoadPinterestStory2024(_BraveLoadingStory):
  NAME = 'load:site:pinterest:2024'
  URL = 'https://www.pinterest.com/search/pins/?q=home%20decor'
  TAGS = [story_tags.YEAR_2024]


class LoadIMDBStory2024(_BraveLoadingStory):
  NAME = 'load:site:imdb:2024'
  URL = 'https://www.imdb.com/what-to-watch/popular/'
  TAGS = [story_tags.YEAR_2024]


class LoadAmazonStory2024(_BraveLoadingStory):
  NAME = 'load:site:amazon:2024'
  URL = 'https://www.amazon.com/gp/new-releases/'
  TAGS = [story_tags.YEAR_2024]


class LoadBraveNewsStory2024(_BraveLoadingStory):
  NAME = 'load:ntp:brave_news:2024'
  URL = 'chrome://newtab/'
  TAGS = [story_tags.YEAR_2024]
