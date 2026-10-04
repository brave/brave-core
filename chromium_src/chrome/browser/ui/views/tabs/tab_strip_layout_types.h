// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_TYPES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_TYPES_H_

// A struct to store the tree height and level of a tab. With this information,
// tabs can be nested in a tree structure.
struct TabNestingInfo {
  int tree_height = 0;
  int level = 0;
};

#include <chrome/browser/ui/views/tabs/tab_strip_layout_types.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_TYPES_H_
