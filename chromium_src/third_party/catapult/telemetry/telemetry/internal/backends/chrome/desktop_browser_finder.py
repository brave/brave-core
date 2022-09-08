# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
"""A inline part of desktop_browser_finder.py"""
import shutil

import override_utils

@override_utils.override_method(PossibleDesktopBrowser)
#def MaybeUpdateSourceProfile(browser_options, profile_directory):
def _TearDownEnvironment(self, original_method):
  if '--update-source-profile' in self._browser_options.extra_browser_args:
      # Override the source profile by the result profile.
      shutil.rmtree(self._browser_options.profile_dir)
      shutil.copytree(self._profile_directory,
                      self._browser_options.profile_dir)
  original_method(self)
