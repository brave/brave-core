/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_SCROLL_CONTAINER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_SCROLL_CONTAINER_H_

#define GetTabStripAvailableWidth                   \
  GetTabStripAvailableWidth_Unused() { return {}; } \
  friend class VerticalTabStripRegionView;          \
  friend class BraveTabStrip;                       \
  int GetTabStripAvailableWidth

#include "src/chrome/browser/ui/views/tabs/tab_strip_scroll_container.h"  // IWYU pragma: export

#undef GetTabStripAvailableWidth

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_SCROLL_CONTAINER_H_
