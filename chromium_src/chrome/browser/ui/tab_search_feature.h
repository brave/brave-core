// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_SEARCH_FEATURE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_SEARCH_FEATURE_H_

#define HasTabSearchToolbarButton \
  HasTabSearchToolbarButton();    \
  bool HasTabSearchToolbarButton_ChromiumImpl

#include <chrome/browser/ui/tab_search_feature.h>  // IWYU pragma: export

#undef HasTabSearchToolbarButton

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_SEARCH_FEATURE_H_
