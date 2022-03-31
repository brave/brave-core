/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_

#define BRAVE_LOCATION_BAR_VIEW_H_   \
 private:                            \
  friend class BraveLocationBarView; \
                                     \
 public:                             \
  virtual std::vector<views::View*> GetTrailingViews();

#define GetBorderRadius virtual GetBorderRadius
#include "src/chrome/browser/ui/views/location_bar/location_bar_view.h"
#undef GetBorderRadius
#undef BRAVE_LOCATION_BAR_VIEW_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_
