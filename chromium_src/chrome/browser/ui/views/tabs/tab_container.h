/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTAINER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTAINER_H_

#define PrepareForAnimation       \
  UnUsed() {}                     \
  friend class BraveTabContainer; \
  void PrepareForAnimation

#define UpdateClosingModeOnRemovedTab virtual UpdateClosingModeOnRemovedTab
#define StartRemoveTabAnimation virtual StartRemoveTabAnimation

#include "src/chrome/browser/ui/views/tabs/tab_container.h"

#undef StartRemoveTabAnimation
#undef UpdateClosingModeOnRemovedTab
#undef PrepareForAnimation

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTAINER_H_
