/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/bookmark_helper.h"

#include "brave/components/constants/pref_names.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave {

BookmarkBarState GetBookmarkBarState(PrefService* prefs) {
  // kShowBookmarkBar has higher priority and the bookmark bar is shown always.
  if (prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar))
    return BookmarkBarState::kAlways;
  // kShowBookmarkBar is false, kAlwaysShowBookmarkBarOnNTP is true
  // -> the bookmark bar is shown only for NTP.
  if (prefs->GetBoolean(kAlwaysShowBookmarkBarOnNTP))
    return BookmarkBarState::kNtp;
  // NEVER show the bookmark bar.
  return BookmarkBarState::kNever;
}

void SetBookmarkState(BookmarkBarState state, PrefService* prefs) {
  if (state == BookmarkBarState::kAlways) {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
    prefs->SetBoolean(kAlwaysShowBookmarkBarOnNTP, false);
  } else if (state == BookmarkBarState::kNtp) {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
    prefs->SetBoolean(kAlwaysShowBookmarkBarOnNTP, true);
  } else {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
    prefs->SetBoolean(kAlwaysShowBookmarkBarOnNTP, false);
  }
}

}  // namespace brave
