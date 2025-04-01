/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_

#define SelectRelativeTab(...)            \
  virtual SelectRelativeTab(__VA_ARGS__); \
  friend class BraveTabStripModel

#define DraggingTabsSession DraggingTabsSessionChromium
#define IsReadLaterSupportedForAny virtual IsReadLaterSupportedForAny

#include "src/chrome/browser/ui/tabs/tab_strip_model.h"  // IWYU pragma: export

#undef IsReadLaterSupportedForAny
#undef DraggingTabsSession
#undef SelectRelativeTab

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
