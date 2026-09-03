/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_bar_controller.h"

#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"

bool ShowBookmarkBarEnabled(PrefService* prefs) {
  return prefs->GetBoolean(::bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP) ||
         prefs->GetBoolean(::bookmarks::prefs::kShowBookmarkBar);
}

#include <chrome/browser/ui/bookmarks/bookmark_bar_controller.cc>
