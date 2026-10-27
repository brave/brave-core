/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BOOKMARK_BOOKMARK_HELPER_H_
#define BRAVE_BROWSER_UI_BOOKMARK_BOOKMARK_HELPER_H_

#include <optional>

namespace ui {
class SimpleMenuModel;
}

class PrefService;

namespace brave {

// Deprecated: we're using bookmarks::BookmarkBarVisibilityState
// instead of ours.

// Show bookmarks a drop down choice of three options:
// - Always (default)
// - Never
// - Only on the new tab page
enum class BookmarkBarState { kAlways, kNever, kNtp };

// Returns true when the bookmarks bar should float over the page.
bool IsFloatingBookmarkBarEnabled(const PrefService& prefs);

// Adds the local floating choice to an existing bookmark visibility submenu.
void AddFloatingBookmarkBarMenuItem(ui::SimpleMenuModel& menu);
bool CanSetFloatingBookmarkBar(const PrefService& prefs);
std::optional<bool> IsFloatingBookmarkBarCommandChecked(
    int command_id,
    const PrefService& prefs);
// Returns true when handled; normal modes continue through native commands.
bool HandleFloatingBookmarkBarCommand(int command_id, PrefService& prefs);

void SetBookmarkStateForTesting(BookmarkBarState state, PrefService* prefs);
void MigrateBookmarkState(PrefService* prefs);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BOOKMARK_BOOKMARK_HELPER_H_
