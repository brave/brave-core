/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_bar_controller.h"

#include "brave/components/constants/pref_names.h"

// We have three bookmarks bar options - Always, Never, Always only on NTP.
// Always only on NTP is our own option.
// As upstream doesn't show bookmarks on NTP when bookmarks bar empty we need
// to patch below code before checking bookmark model's emptiness.
//
// Also, bookmarks bar on NTP should be shown when "Always" and "Always only on
// NTP". With SideBySide, bookmarks bar is shown when one of split tab is NTP.
#define BRAVE_BOOKMARK_BAR_CONTROLLER_SHOULD_SHOW_BOOKMARK_BAR              \
  if (const tabs::TabInterface* active_tab =                                \
          tab_strip_model_->GetActiveTab()) {                               \
    if (active_tab->GetContents() &&                                        \
        active_tab->GetContents()->IsFullscreen()) {                        \
      return false;                                                         \
    }                                                                       \
    return std::any_of(                                                     \
        tabs.begin(), tabs.end(), [&prefs](const tabs::TabInterface* tab) { \
          return tab->GetContents() && IsShowingNTP(tab->GetContents()) &&  \
                 (prefs->GetBoolean(kAlwaysShowBookmarkBarOnNTP) ||         \
                  prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar));   \
        });                                                                 \
  }

#include <chrome/browser/ui/bookmarks/bookmark_bar_controller.cc>

#undef BRAVE_BOOKMARK_BAR_CONTROLLER_SHOULD_SHOW_BOOKMARK_BAR

bool IsShowingNTP_ChromiumImpl(content::WebContents* web_contents) {
  return IsShowingNTP(web_contents);
}
