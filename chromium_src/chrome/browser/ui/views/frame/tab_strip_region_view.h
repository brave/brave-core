/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_TAB_STRIP_REGION_VIEW_H_

#include "base/gtest_prod_util.h"

#define IsPositionInWindowCaption                                   \
  Unused_IsPositionInWindowCaption() {                              \
    return false;                                                   \
  }                                                                 \
  friend class VerticalTabStripRegionView;                          \
  friend class BraveTabStrip;                                       \
  friend class BraveTabStripRegionView;                             \
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, MinHeight); \
  bool IsPositionInWindowCaption

#include "src/chrome/browser/ui/views/frame/tab_strip_region_view.h"  // IWYU pragma: export

#undef IsPositionInWindowCaption

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_TAB_STRIP_REGION_VIEW_H_
