/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STYLE_VIEWS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STYLE_VIEWS_H_

#define CreateForTab CreateForTab_ChromiumImpl(Tab* tab); \
  static std::unique_ptr<TabStyleViews> CreateForTab

#include "src/chrome/browser/ui/views/tabs/tab_style_views.h"  // IWYU pragma: export
#undef CreateForTab

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STYLE_VIEWS_H_
