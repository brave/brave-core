/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_bar_controller.h"

#include "brave/components/constants/pref_names.h"

// We have one more option for bookmarks bar visibility.
// It's "only show bookmarks bar on NTP". And it's patched to prevent
// checking split view. Chromium shows bookmarks bar if active tab is
// in split tabs and split tabs includes NTP. But, we don't want to show
// bookmarks bar if that active tab is not NTP.
// It's inevitable to copy some upstream code when overriding this method.
// Also added this before checking bookmarks/tab groups existence.
// We show bookmarks bar on NTP if there is no bookmarks/tab groups.
#define BRAVE_BOOKMARK_BAR_CONTROLLER_SHOULD_SHOW_BOOKMARK_BAR \
  return IsShowingNTP(active_tab->GetContents()) &&            \
         prefs->GetBoolean(kAlwaysShowBookmarkBarOnNTP);

#include "src/chrome/browser/ui/bookmarks/bookmark_bar_controller.cc"

#undef BRAVE_BOOKMARK_BAR_CONTROLLER_SHOULD_SHOW_BOOKMARK_BAR

bool IsShowingNTP_ChromiumImpl(content::WebContents* web_contents) {
  return IsShowingNTP(web_contents);
}
