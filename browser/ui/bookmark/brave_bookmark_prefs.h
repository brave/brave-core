/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BOOKMARK_BRAVE_BOOKMARK_PREFS_H_
#define BRAVE_BROWSER_UI_BOOKMARK_BRAVE_BOOKMARK_PREFS_H_

class PrefRegistrySimple;

namespace brave::bookmarks::prefs {

inline constexpr char kShowAllBookmarksButton[] =
    "brave.bookmarks.show_all_bookmarks_button";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace brave::bookmarks::prefs

#endif  // BRAVE_BROWSER_UI_BOOKMARK_BRAVE_BOOKMARK_PREFS_H_
