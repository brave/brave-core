/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_PREFS_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_PREFS_H_

class PrefRegistrySimple;
class PrefService;

namespace brave_tabs {

enum TabHoverMode { TOOLTIP = 0, CARD = 1, CARD_WITH_PREVIEW = 2 };

inline constexpr char kTabHoverMode[] = "brave.tabs.hover_mode";

inline constexpr char kVerticalTabsEnabled[] =
    "brave.tabs.vertical_tabs_enabled";
inline constexpr char kVerticalTabsCollapsed[] =
    "brave.tabs.vertical_tabs_collapsed";
inline constexpr char kVerticalTabsExpandedStatePerWindow[] =
    "brave.tabs.vertical_tabs_expanded_state_per_window";
inline constexpr char kVerticalTabsShowTitleOnWindow[] =
    "brave.tabs.vertical_tabs_show_title_on_window";
inline constexpr char kVerticalTabsFloatingEnabled[] =
    "brave.tabs.vertical_tabs_floating_enabled";
inline constexpr char kVerticalTabsExpandedWidth[] =
    "brave.tabs.vertical_tabs_expanded_width";
inline constexpr char kVerticalTabsOnRight[] =
    "brave.tabs.vertical_tabs_on_right";
inline constexpr char kVerticalTabsShowScrollbar[] =
    "brave.tabs.vertical_tabs_show_scrollbar";

inline constexpr char kSharedPinnedTab[] = "brave.tabs.shared_pinned_tab";

void RegisterBraveProfilePrefs(PrefRegistrySimple* registry);
void MigrateBraveProfilePrefs(PrefService* prefs);

bool AreTooltipsEnabled(PrefService* prefs);
bool AreCardPreviewsEnabled(PrefService* prefs);

}  // namespace brave_tabs

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_PREFS_H_
