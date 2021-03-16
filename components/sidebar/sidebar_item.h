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

  static SidebarItem Create(const GURL& url,
                            const std::u16string& title,
                            Type type,
                            bool open_in_panel);

  SidebarItem();
  ~SidebarItem();

  GURL url;
  Type type;
  std::u16string title;
  // Set false to open this item in new tab.
  bool open_in_panel;
};

bool IsBuiltInType(const SidebarItem& item);
bool IsWebType(const SidebarItem& item);

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_ITEM_H_
