// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TAB_SEARCH_BUBBLE_HOST_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TAB_SEARCH_BUBBLE_HOST_H_

// Bubble should use different border arrow per tab position.
// Set TOP_RIGHT in horizontal tab and TOP_LEFT in vertical tab
// See TabSearchBubbleHost::ShowTabSearchBubble().
// Didn't change GetTabSearchPosition() because it's called from many
// places and could add unexpected regressions.
// Simply added one boolean flag.
#define CloseTabSearchBubble            \
  set_use_brave_vertical_tab() {        \
    use_brave_vertical_tab_ = true;     \
  }                                     \
  bool use_brave_vertical_tab() const { \
    return use_brave_vertical_tab_;     \
  }                                     \
                                        \
 private:                               \
  bool use_brave_vertical_tab_ = false; \
                                        \
 public:                                \
  void CloseTabSearchBubble

#include <chrome/browser/ui/views/tab_search_bubble_host.h>  // IWYU pragma: export

#undef CloseTabSearchBubble

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TAB_SEARCH_BUBBLE_HOST_H_
