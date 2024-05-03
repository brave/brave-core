/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_RECENT_TABS_SUB_MENU_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_RECENT_TABS_SUB_MENU_MODEL_H_

#include <memory>

#define local_tab_cancelable_task_tracker_         \
  local_tab_cancelable_task_tracker_;              \
  std::unique_ptr<sessions::SessionTab> stub_tab_; \
  friend class BraveRecentTabsSubMenuModel

#include "src/chrome/browser/ui/tabs/recent_tabs_sub_menu_model.h"  // IWYU pragma: export

#undef local_tab_cancelable_task_tracker_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_RECENT_TABS_SUB_MENU_MODEL_H_
