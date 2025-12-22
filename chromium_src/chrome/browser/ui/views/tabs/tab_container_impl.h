/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTAINER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTAINER_IMPL_H_

#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"

#define ExitTabClosingMode        \
  UnUsed() {}                     \
  friend class BraveTabContainer; \
  void ExitTabClosingMode

#define UpdateClosingModeOnRemovedTab virtual UpdateClosingModeOnRemovedTab
#define GetTargetBoundsForClosingTab virtual GetTargetBoundsForClosingTab
#define StartInsertTabAnimation virtual StartInsertTabAnimation
#define ShouldTabBeVisible virtual ShouldTabBeVisible
#define IsPointInTab virtual IsPointInTab
#define UpdateIdealBounds virtual UpdateIdealBounds

#include <chrome/browser/ui/views/tabs/tab_container_impl.h>  // IWYU pragma: export

#undef UpdateIdealBounds
#undef IsPointInTab
#undef ShouldTabBeVisible
#undef StartInsertTabAnimation
#undef GetTargetBoundsForClosingTab
#undef UpdateClosingModeOnRemovedTab
#undef ExitTabClosingMode

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTAINER_IMPL_H_
