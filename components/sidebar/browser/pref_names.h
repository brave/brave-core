/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_SIDEBAR_BROWSER_PREF_NAMES_H_

namespace sidebar {

inline constexpr char kSidebarItems[] = "brave.sidebar.sidebar_items";
inline constexpr char kSidebarHiddenBuiltInItems[] =
    "brave.sidebar.hidden_built_in_items";
inline constexpr char kSidebarShowOption[] =
    "brave.sidebar.sidebar_show_option";
inline constexpr char kSidebarItemAddedFeedbackBubbleShowCount[] =
    "brave.sidebar.item_added_feedback_bubble_shown_count";
inline constexpr char kSidePanelWidth[] = "brave.sidebar.side_panel_width";
inline constexpr char kLastUsedBuiltInItemType[] =
    "brave.sidebar.last_used_built_in_item_type";

// Indicates that the Leo side panel is opened once for Sidebar Enabled-by
// -default test via griffin.
inline constexpr char kLeoPanelOneShotOpen[] =
    "brave.sidebar.leo_panel_one_shot_shot";

// Indicates that it's target user for Sidebar Enabled-by-default test via
// griffin. local state pref.
inline constexpr char kTargetUserForSidebarEnabledTest[] =
    "brave.sidebar.target_user_for_sidebar_enabled_test";

// Indicates that sidebar alignment was changed by the browser itself, not by
// users.
inline constexpr char kSidebarAlignmentChangedTemporarily[] =
    "brave.sidebar.sidebar_alignment_changed_for_vertical_tabs";

inline constexpr char kSidebarSettingChangeInitialP3AReport[] =
    "brave.sidebar.setting_change_initial_p3a_reported";

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_BROWSER_PREF_NAMES_H_
