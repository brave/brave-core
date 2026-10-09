/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/browser/sidebar_item.h"

namespace sidebar {

// static
SidebarItem SidebarItem::Create(const std::u16string& title,
                                Type type,
                                BuiltInItemType built_in_item_type,
                                PanelType panel_type) {
  SidebarItem item;
  item.title = title;
  item.type = type;
  item.built_in_item_type = built_in_item_type;
  item.panel_type = panel_type;
  return item;
}

// static
SidebarItem SidebarItem::Create(const GURL& url,
                                const std::u16string& title,
                                Type type,
                                BuiltInItemType built_in_item_type,
                                PanelType panel_type) {
  SidebarItem item = Create(title, type, built_in_item_type, panel_type);
  item.url = url;
  return item;
}

SidebarItem::SidebarItem() = default;
SidebarItem::SidebarItem(const SidebarItem&) = default;
SidebarItem& SidebarItem::operator=(const SidebarItem&) = default;
SidebarItem::SidebarItem(SidebarItem&&) = default;
SidebarItem& SidebarItem::operator=(SidebarItem&&) = default;

SidebarItem::~SidebarItem() = default;

bool SidebarItem::operator==(const SidebarItem& item) const {
  return url == item.url && title == item.title && type == item.type &&
         built_in_item_type == item.built_in_item_type &&
         panel_type == item.panel_type;
}

bool SidebarItem::IsValidItem() const {
  // Any type should have valid title.
  if (title.empty()) {
    return false;
  }

  if (type == SidebarItem::Type::kTypeBuiltIn) {
    return built_in_item_type != SidebarItem::BuiltInItemType::kNone;
  }

  // WebType
  return url.is_valid() &&
         built_in_item_type == SidebarItem::BuiltInItemType::kNone;
}

}  // namespace sidebar
