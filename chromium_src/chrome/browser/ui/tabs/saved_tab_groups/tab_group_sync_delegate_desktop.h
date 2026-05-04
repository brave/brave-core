/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_SAVED_TAB_GROUPS_TAB_GROUP_SYNC_DELEGATE_DESKTOP_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_SAVED_TAB_GROUPS_TAB_GROUP_SYNC_DELEGATE_DESKTOP_H_

// Pre-include all headers that reference ConnectLocalTabGroup before the
// macro so their include guards prevent the macro from affecting them.
#include "components/saved_tab_groups/delegate/tab_group_sync_delegate.h"
#include "components/saved_tab_groups/public/tab_group_sync_service.h"

#define ConnectLocalTabGroup                                     \
  ConnectLocalTabGroup_ChromiumImpl(const SavedTabGroup& group); \
  void ConnectLocalTabGroup

#include <chrome/browser/ui/tabs/saved_tab_groups/tab_group_sync_delegate_desktop.h>  // IWYU pragma: export

#undef ConnectLocalTabGroup

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_SAVED_TAB_GROUPS_TAB_GROUP_SYNC_DELEGATE_DESKTOP_H_
