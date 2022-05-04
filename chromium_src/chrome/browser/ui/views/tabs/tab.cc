/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab.h"

// Set alert indicator's pos to start of the title and
// move title after the alert indicator.
// Title right should respect close btn's space
#define BRAVE_UI_VIEWS_TABS_TAB_ALERT_INDICATOR_POSITION           \
  alert_indicator_button_->SetX(title_left - after_title_padding); \
  title_right = close_x - after_title_padding;                     \
  if (showing_close_button_)                                       \
    title_right = close_x - after_title_padding;                   \
  title_left = alert_indicator_button_->x() +                      \
               alert_indicator_button_->width() + after_title_padding;

#define BRAVE_UI_VIEWS_TABS_TAB_UPDATE_ICON_VISIBILITY \
  showing_close_button_ &= mouse_hovered();

#define GetWidthOfLargestSelectableRegion \
  GetWidthOfLargestSelectableRegion_ChromiumImpl
#include "src/chrome/browser/ui/views/tabs/tab.cc"
#undef GetWidthOfLargestSelectableRegion
#undef BRAVE_UI_VIEWS_TABS_TAB_UPDATE_ICON_VISIBILITY
#undef BRAVE_UI_VIEWS_TABS_TAB_ALERT_INDICATOR_POSITION

// Re-defined because we moved alert button to left side in the tab whereas
// upstream put it on right side. Need to consider this change for calculating
// largest selectable region.
int Tab::GetWidthOfLargestSelectableRegion() const {
  // Assume the entire region except the area that alert indicator/close buttons
  // occupied is available for click-to-select.
  // If neither are visible, the entire tab region is available.
  int selectable_width = width();
  if (alert_indicator_button_->GetVisible()) {
    selectable_width -= alert_indicator_button_->width();
  }

  if (close_button_->GetVisible())
    selectable_width -= close_button_->width();

  return std::max(0, selectable_width);
}
