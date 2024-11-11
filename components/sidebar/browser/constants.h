/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_BROWSER_CONSTANTS_H_
#define BRAVE_COMPONENTS_SIDEBAR_BROWSER_CONSTANTS_H_

namespace sidebar {

inline constexpr char kSidebarItemURLKey[] = "url";
inline constexpr char kSidebarItemTypeKey[] = "type";
inline constexpr char kSidebarItemBuiltInItemTypeKey[] = "built_in_item_type";
inline constexpr char kSidebarItemTitleKey[] = "title";
inline constexpr char kSidebarItemOpenInPanelKey[] = "open_in_panel";
inline constexpr int kDefaultSidePanelWidth = 320;

// list is provided from chrome layer.
inline constexpr char kBraveTalkURL[] = "https://talk.brave.com/widget";
inline constexpr char kBraveTalkHost[] = "talk.brave.com";

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_BROWSER_CONSTANTS_H_
