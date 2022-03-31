/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_CONSTANTS_H_
#define BRAVE_COMPONENTS_SIDEBAR_CONSTANTS_H_

namespace sidebar {

constexpr char kSidebarItemURLKey[] = "url";
constexpr char kSidebarItemTypeKey[] = "type";
constexpr char kSidebarItemBuiltInItemTypeKey[] = "built_in_item_type";
constexpr char kSidebarItemTitleKey[] = "title";
constexpr char kSidebarItemOpenInPanelKey[] = "open_in_panel";

// TODO(simonhong): Move this to //brave/common/webui_url_constants.h when
// default builtin items list is provided from chrome layer.
constexpr char kSidebarBookmarksURL[] =
    "chrome://sidebar-bookmarks.top-chrome/";

constexpr char kBraveTalkURL[] = "https://talk.brave.com/widget";
constexpr char kBraveTalkHost[] = "talk.brave.com";

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_CONSTANTS_H_
