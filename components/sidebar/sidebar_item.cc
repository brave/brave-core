/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/sidebar_item.h"

namespace sidebar {

// static
SidebarItem SidebarItem::Create(const GURL& url,
                                const std::u16string& title,
                                Type type,
                                bool open_in_panel) {
  SidebarItem item;
  item.url = url;
  item.title = title;
  item.type = type;
  item.open_in_panel = open_in_panel;
  return item;
}

SidebarItem::SidebarItem() = default;

SidebarItem::~SidebarItem() = default;

bool IsBuiltInType(const SidebarItem& item) {
  return item.type == SidebarItem::Type::kTypeBuiltIn;
}

bool IsWebType(const SidebarItem& item) {
  return item.type == SidebarItem::Type::kTypeWeb;
}

}  // namespace sidebar
