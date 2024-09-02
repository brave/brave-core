# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A inline part of desktop_browser_finder.py"""

import override_utils


@override_utils.override_method(PossibleDesktopBrowser)
def GetBrowserStartupArgs(self, original_method, browser_options):
    startup_args = original_method(self, browser_options)
    # Don't disable chromium component by default.
    startup_args.remove("--disable-component-update")
    return startup_args
