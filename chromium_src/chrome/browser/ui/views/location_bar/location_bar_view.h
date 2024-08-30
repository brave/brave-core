/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_

#define BRAVE_LOCATION_BAR_VIEW_H_                               \
 private:                                                        \
  friend class BraveLocationBarView;                             \
                                                                 \
 public:                                                         \
  virtual views::View* GetSearchPromotionButton() const;         \
  virtual std::vector<views::View*> GetRightMostTrailingViews(); \
  virtual std::vector<views::View*> GetLeftMostTrailingViews();

#define OnOmniboxBlurred virtual OnOmniboxBlurred
#define GetBorderRadius virtual GetBorderRadius
#define RefreshBackground virtual RefreshBackground
#include "src/chrome/browser/ui/views/location_bar/location_bar_view.h"  // IWYU pragma: export
#undef RefreshBackground
#undef GetBorderRadius
#undef OnOmniboxBlurred
#undef BRAVE_LOCATION_BAR_VIEW_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_
