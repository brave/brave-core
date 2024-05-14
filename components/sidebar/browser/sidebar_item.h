/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_BROWSER_SIDEBAR_ITEM_H_
#define BRAVE_COMPONENTS_SIDEBAR_BROWSER_SIDEBAR_ITEM_H_

#include "url/gurl.h"

namespace sidebar {

struct SidebarItem {
  enum class Type {
    kTypeBuiltIn,
    kTypeWeb,
  };

  // Do not reorder or remove items, as underlying values are used as id of
  // items.
  enum class BuiltInItemType {
    kNone = 0,
    kBraveTalk,
    kWallet,
    kBookmarks,
    kReadingList,
    kHistory,
    kPlaylist,
    kChatUI,
    // When adding new item, dont' forget to update kBuiltInItemLast.
    kBuiltInItemLast = kChatUI,
  };

  static SidebarItem Create(const std::u16string& title,
                            Type type,
                            BuiltInItemType built_in_item_type,
                            bool open_in_panel);

  static SidebarItem Create(const GURL& url,
                            const std::u16string& title,
                            Type type,
                            BuiltInItemType built_in_item_type,
                            bool open_in_panel);

  SidebarItem();
  SidebarItem(const SidebarItem&);
  SidebarItem& operator=(const SidebarItem&);
  SidebarItem(SidebarItem&&);
  SidebarItem& operator=(SidebarItem&&);
  ~SidebarItem();

  bool operator==(const SidebarItem& item) const;

  GURL url;
  Type type = Type::kTypeBuiltIn;
  BuiltInItemType built_in_item_type = BuiltInItemType::kNone;
  std::u16string title;
  // Set false to open this item in new tab.
  bool open_in_panel = false;
};

bool IsBuiltInType(const SidebarItem& item);
bool IsWebType(const SidebarItem& item);
bool IsValidItem(const SidebarItem& item);

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_BROWSER_SIDEBAR_ITEM_H_
