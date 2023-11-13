/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab.h"

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/layout_constants.h"

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

// `UpdateIconVisibility` currently has an early return when the tab view's
// height is less than `GetLayoutConstant(TAB_HEIGHT)`. Unfortunately, when in
// vertical tabs mode this will prevent the favicon and close button from
// appearing. As a workaround, use `tabs::kVerticalTabHeight` instead of
// TAB_HEIGHT when in vertical tabs mode.
#define GetLayoutConstant(COMPONENT)                                 \
  ((COMPONENT == TAB_HEIGHT &&                                       \
    tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) \
       ? tabs::kVerticalTabHeight                                    \
       : GetLayoutConstant(COMPONENT))

#include "src/chrome/browser/ui/views/tabs/tab.cc"

#undef GetLayoutConstant
#undef BRAVE_UI_VIEWS_TABS_TAB_UPDATE_ICON_VISIBILITY
#undef BRAVE_UI_VIEWS_TABS_TAB_ALERT_INDICATOR_POSITION
