// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SIDE_PANEL_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SIDE_PANEL_UTILS_H_

#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"

namespace sidebar {

struct SidebarItem;

SidePanelEntry::Id SidePanelIdFromSideBarItem(const SidebarItem& item);

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SIDE_PANEL_UTILS_H_
