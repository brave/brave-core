/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_WIDTH_CONSTRAINTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_WIDTH_CONSTRAINTS_H_

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"

#define TransformForPinnednessAndOpenness      \
  TransformForPinnednessAndOpenness_UnUsed() { \
    return {};                                 \
  }                                            \
                                               \
 public:                                       \
  void set_is_tab_in_group(bool in_group) {    \
    is_tab_in_group_ = in_group;               \
  }                                            \
  bool is_tab_in_group() const {               \
    return is_tab_in_group_;                   \
  }                                            \
  const TabLayoutState& state() const {        \
    return state_;                             \
  }                                            \
  TabLayoutState& state() {                    \
    return state_;                             \
  }                                            \
                                               \
 private:                                      \
  bool is_tab_in_group_ = false;               \
  float TransformForPinnednessAndOpenness

#include "src/chrome/browser/ui/views/tabs/tab_width_constraints.h"  // IWYU pragma: export

#undef TransformForPinnednessAndOpenness

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_WIDTH_CONSTRAINTS_H_
