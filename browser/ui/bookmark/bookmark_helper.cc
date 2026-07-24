/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/bookmark_helper.h"

#include "brave/components/constants/pref_names.h"
#include "components/bookmarks/common/bookmark_bar_visibility_state.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

BookmarkBarState GetBookmarkBarState(PrefService* prefs) {
  // kShowBookmarkBar has higher priority and the bookmark bar is shown always.
  if (prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar))
    return BookmarkBarState::kAlways;
  // kShowBookmarkBar is false, kAlwaysShowBookmarkBarOnNTP is true
  // -> the bookmark bar is shown only for NTP.
  if (prefs->GetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP)) {
    return BookmarkBarState::kNtp;
  }
  // NEVER show the bookmark bar.
  return BookmarkBarState::kNever;
}

}  // namespace

void SetBookmarkStateForTesting(BookmarkBarState state, PrefService* prefs) {
  if (state == BookmarkBarState::kAlways) {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
    prefs->SetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP, false);
  } else if (state == BookmarkBarState::kNtp) {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
    prefs->SetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP, true);
  } else {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
    prefs->SetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP, false);
  }
}

// Migrate to use upstream's kBookmarkBarVisibilityState.
void MigrateBookmarkState(PrefService* prefs) {
  // Nothing to migrate. Clean kShowBookmarkBar means
  // kAlwaysShowBookmarkBarOnNTP is also default state.
  if (auto* pref =
          prefs->FindPreference(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP);
      pref->IsDefaultValue()) {
    return;
  }

  bookmarks::BookmarkBarVisibilityState state =
      bookmarks::BookmarkBarVisibilityState::kAlwaysHide;
  const BookmarkBarState deprecated_state = GetBookmarkBarState(prefs);
  if (deprecated_state == BookmarkBarState::kAlways) {
    state = bookmarks::BookmarkBarVisibilityState::kAlwaysShow;
  } else if (deprecated_state == BookmarkBarState::kNtp) {
    state = bookmarks::BookmarkBarVisibilityState::kOnlyShowOnNtp;
  }
  prefs->SetInteger(bookmarks::prefs::kBookmarkBarVisibilityState,
                    static_cast<int>(state));

  // Didn't clear kShowBookmarkBar as it's upstream prefs and it's still
  // synced with kBookmarkBarVisibilityState.
  prefs->ClearPref(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP);
}

}  // namespace brave
