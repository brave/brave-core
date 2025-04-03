/* Copyright (c) 2015 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ICON_LABEL_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ICON_LABEL_BUBBLE_VIEW_H_

class BraveLocationBarView;

// Making this class friend of `BraveLocationBarView`, so `GetMinimumSize` can
// be called for its icons, as this function is now overridden in
// `IconLabelBubbleView` and made protected.
#define UpdateLabelColors            \
  UpdateLabelColors_Unused();        \
  friend class BraveLocationBarView; \
  void UpdateLabelColors

#include "src/chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"  // IWYU pragma: export

#undef UpdateLabelColors

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ICON_LABEL_BUBBLE_VIEW_H_
