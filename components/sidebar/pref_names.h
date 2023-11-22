/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_PREF_NAMES_H_
#define BRAVE_COMPONENTS_SIDEBAR_PREF_NAMES_H_

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

// Indicates that sidebar alignment was changed by the browser itself, not by
// users.
inline constexpr char kSidebarAlignmentChangedTemporarily[] =
    "brave.sidebar.sidebar_alignment_changed_for_vertical_tabs";

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_PREF_NAMES_H_
