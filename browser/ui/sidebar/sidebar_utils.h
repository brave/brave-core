/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_

#include "brave/components/sidebar/sidebar_item.h"

class Browser;
class GURL;
enum class SidePanelEntryId;

namespace sidebar {

class SidebarService;

bool CanUseSidebar(Browser* browser);
bool CanAddCurrentActiveTabToSidebar(Browser* browser);

// Exported for testing.
bool HiddenDefaultSidebarItemsContains(SidebarService* service,
                                       const GURL& url);
GURL ConvertURLToBuiltInItemURL(const GURL& url);
SidePanelEntryId SidePanelIdFromSideBarItemType(
    SidebarItem::BuiltInItemType type);
SidePanelEntryId SidePanelIdFromSideBarItem(const SidebarItem& item);

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_
