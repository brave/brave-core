/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_bar_controller.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/ui/sad_tab.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

bool ShowBookmarkBarEnabled(content::WebContents* web_contents) {
  PrefService* prefs =
      user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
  return prefs->GetBoolean(kAlwaysShowBookmarkBarOnNTP) ||
         prefs->GetBoolean(::bookmarks::prefs::kShowBookmarkBar);
}

#define ShouldShow(...) \
  ShouldShow(__VA_ARGS__) || !ShowBookmarkBarEnabled(web_contents)

#include <chrome/browser/ui/bookmarks/bookmark_bar_controller.cc>

#undef ShouldShow
