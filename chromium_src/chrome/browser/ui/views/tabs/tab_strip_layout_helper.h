/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_H_

#define UpdateIdealBounds                                                      \
  UnUsed() { return {}; }                                                      \
  void set_use_vertical_tabs(bool vertical) { use_vertical_tabs_ = vertical; } \
                                                                               \
 private:                                                                      \
  bool use_vertical_tabs_ = false;                                             \
                                                                               \
 public:                                                                       \
  int UpdateIdealBounds

#include "src/chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"

#undef UpdateIdealBounds

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_H_
