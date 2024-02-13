// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTIL_H_

#define PopulateGlobalEntries                                             \
  PopulateGlobalEntries_ChromiumImpl(Browser* browser,                    \
                                     SidePanelRegistry* global_registry); \
  static void PopulateGlobalEntries

#include "src/chrome/browser/ui/views/side_panel/side_panel_util.h"  // IWYU pragma: export

#undef PopulateGlobalEntries

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTIL_H_
