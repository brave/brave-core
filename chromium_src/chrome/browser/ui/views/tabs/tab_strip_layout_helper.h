/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_H_

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"

class BraveTabStrip;

// As TabStripLayoutHelper's destructor is not virtual, it'd be safer not to
// make virtual methods and child class of it
#define UpdateIdealBounds                                           \
  UnUsed() {                                                        \
    return {};                                                      \
  }                                                                 \
  void set_use_vertical_tabs(bool vertical) {                       \
    use_vertical_tabs_ = vertical;                                  \
  }                                                                 \
  void set_tab_strip(TabStrip* tab_strip) {                         \
    tab_strip_ = tab_strip;                                         \
  }                                                                 \
                                                                    \
 private:                                                           \
  friend class BraveTabContainer;                                   \
  bool FillGroupInfo(std::vector<TabWidthConstraints>& tab_widths); \
  BraveTabStrip* GetBraveTabStrip() const;                          \
  bool FillTiledState(std::vector<TabWidthConstraints>& tab_widths, \
                      BraveTabStrip* tab_strip);                    \
  bool use_vertical_tabs_ = false;                                  \
  raw_ptr<TabStrip> tab_strip_ = nullptr;                           \
                                                                    \
 public:                                                            \
  int UpdateIdealBounds

#include "src/chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"  // IWYU pragma: export

#undef UpdateIdealBounds

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_H_
