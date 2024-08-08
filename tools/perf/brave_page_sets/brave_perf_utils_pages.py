# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# A page set for brave_utils benchmark.
#
# Used as part of chromium src/tools/perf/page_sets/
# Uses the the same code conventions (including pylint).
# pylint: disable=import-error, no-self-use
# pylint: disable=no-name-in-module, too-few-public-methods
# pytype: disable=import-error

import logging
import shutil
import os
import time

from typing import Optional

from telemetry.page import shared_page_state
from telemetry.page import page as page_module
from telemetry import story
from telemetry.core import android_platform


class _UpdateProfileSharedPageState(shared_page_state.SharedPageState):
  """ A special utility state to update a source profile.

  It stores the result profile as the source after the test is finished.
  It actually modifies the behavior of SharedPageState, so it uses
  the private members of the classes.

  Can be used with --use-live-sites or without.
  """

  _profile_dir: Optional[str]

  def __init__(self, test, finder_options, story_set, possible_browser):
    super().__init__(test, finder_options, story_set, possible_browser)
    self._profile_dir = self.browser_options().profile_dir

  def _StartBrowser(self, page):
    # Update the source profile for desktop (see desktop_browser_finder.py)
    self.browser_options().profile_type = 'exact'

    super()._StartBrowser(page)

  def _StopBrowser(self):
    is_android = self._possible_browser.platform.GetOSName() == 'android'
    if self.browser:
      self.browser.Close()

    if self._profile_dir is not None:
      if is_android:
        self._PullAndUpdateAndroidProfile()
      else:
        # profile_type = 'exact' is used to update the profile
        pass
      time.sleep(5)
      self._profile_dir = None

    super()._StopBrowser()

  def browser_options(self):
    return self._finder_options.browser_options

  def _PullAndUpdateAndroidProfile(self):
    assert isinstance(self.platform, android_platform.AndroidPlatform)
    possible_browser = self._possible_browser

    # pylint: disable=protected-access # get device without patching
    platform_backend = possible_browser._platform_backend
    # pylint: enable=protected-access

    device = platform_backend.device
    assert device is not None

    dest_profile = self.browser_options().profile_dir
    assert dest_profile is not None

    # Remove the current profile
    shutil.rmtree(dest_profile)

    logging.info('Pull updated profile from %s to %s',
                 possible_browser.profile_directory, dest_profile)
    device.PullFile(possible_browser.profile_directory, dest_profile, True)


class _UpdateProfilePage(page_module.Page):
  _delay: int

  def __init__(self, page_set, delay: int):
    EXTRA_BROWSER_ARGUMENTS = ['--enable-brave-features-for-perf-testing']
    self._delay = delay
    super().__init__(url='chrome://components',
                     page_set=page_set,
                     shared_page_state_class=_UpdateProfileSharedPageState,
                     name='UpdateProfile',
                     extra_browser_args=EXTRA_BROWSER_ARGUMENTS)

  def RunPageInteractions(self, action_runner):
    action_runner.Wait(self._delay)
    if action_runner.tab.browser.platform.GetOSName() != 'android':
      # Disable session restore via settingsPrivate API.
      t = action_runner.tab
      t.Navigate('chrome://settings')
      t.WaitForDocumentReadyStateToBeInteractiveOrBetter()
      t.EvaluateJavaScript(
          'chrome.settingsPrivate.setPref("session.restore_on_startup", 5)')
      t.EvaluateJavaScript(
          'chrome.settingsPrivate.setPref("intl.accept_languages", "en-US")')
      t.EvaluateJavaScript(
          'chrome.settingsPrivate.setPref("intl.selected_languages", "en-US")')
      action_runner.Wait(2)


class BravePerfUtilsStorySet(story.StorySet):
  """ Brave version of LoadingDesktopStorySet.

  See loading_desktop.py for details.
  """

  def __init__(self, delay=10):
    super().__init__(cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(_UpdateProfilePage(self, delay))
