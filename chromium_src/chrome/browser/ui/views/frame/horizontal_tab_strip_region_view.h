/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_HORIZONTAL_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_HORIZONTAL_TAB_STRIP_REGION_VIEW_H_

#include "base/gtest_prod_util.h"

#define IsPositionInWindowCaption                                   \
  Unused_IsPositionInWindowCaption() {                              \
    return false;                                                   \
  }                                                                 \
  views::Button* new_tab_button() {                                 \
    return new_tab_button_;                                         \
  }                                                                 \
  friend class BraveVerticalTabStripRegionView;                     \
  friend class BraveTabStrip;                                       \
  friend class BraveHorizontalTabStripRegionView;                   \
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, MinHeight); \
  bool IsPositionInWindowCaption

#define UpdateTabStripMargin virtual UpdateTabStripMargin
// Function-like macro (note the parentheses after the macro name): expands
// only at the declaration site `void UpdateButtonBorders();` and not at the
// pointer-to-member reference
// `&HorizontalTabStripRegionView::UpdateButtonBorders` further down in the
// upstream header. An object-like
// `#define UpdateButtonBorders virtual UpdateButtonBorders` would also rewrite
// the pointer-to-member into `&...::virtual UpdateButtonBorders`, which is a
// syntax error.
#define UpdateButtonBorders() virtual UpdateButtonBorders()

#include <chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h>  // IWYU pragma: export

#undef UpdateButtonBorders
#undef UpdateTabStripMargin
#undef IsPositionInWindowCaption

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_HORIZONTAL_TAB_STRIP_REGION_VIEW_H_
