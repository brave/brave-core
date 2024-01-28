/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_ITEM_H_
#define BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_ITEM_H_

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

  bool CanOpenInPanel() const;
  bool IsBuiltInType() const;
  bool IsWebType() const;
  bool IsValidItem() const;
  bool IsMobileViewItem() const;

  bool operator==(const SidebarItem& item) const;

  GURL url;
  Type type = Type::kTypeBuiltIn;
  BuiltInItemType built_in_item_type = BuiltInItemType::kNone;
  std::u16string title;
  // Set false to open this item in new tab.
  bool open_in_panel = false;

  // TODO(simonhong): Remove this and migrate to |open_in_panel|.
  // As mobile view feature can be toggled, |open_in_panel| flag
  // should be preserved till this feature flag is removed.
  bool mobile_view = false;
};

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_ITEM_H_
