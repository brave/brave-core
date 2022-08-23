// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/sidebar/sidebar_side_panel_utils.h"

#include "base/logging.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"

namespace sidebar {

using BuiltInItemType = SidebarItem::BuiltInItemType;

SidePanelEntry::Id SidePanelIdFromSideBarItem(const SidebarItem& item) {
  DCHECK(item.open_in_panel);
  switch (item.built_in_item_type) {
    case BuiltInItemType::kReadingList:
      return SidePanelEntry::Id::kReadingList;
    case BuiltInItemType::kBookmarks:
      return SidePanelEntry::Id::kBookmarks;
    default:
      // Add a new case for any new types which we want to support.
      NOTREACHED() << "Asked for a panel Id from a sidebar item which should "
                      "not have a panel Id, sending Reading List instead.";
      return SidePanelEntry::Id::kReadingList;
  }
}

}  // namespace sidebar
