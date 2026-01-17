/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_

#include "chrome/browser/ui/views/location_bar/content_setting_image_view.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

#define GetBackgroundColorForTesting                             \
  Unused();                                                      \
                                                                 \
 private:                                                        \
  friend class BraveLocationBarView;                             \
                                                                 \
 public:                                                         \
  virtual views::View* GetSearchPromotionButton() const;         \
  virtual std::vector<views::View*> GetRightMostTrailingViews(); \
  virtual std::vector<views::View*> GetLeftMostTrailingViews();  \
  SkColor GetBackgroundColorForTesting

#define Init virtual Init
#define OnOmniboxBlurred virtual OnOmniboxBlurred
#define GetBorderRadius virtual GetBorderRadius
#define RefreshBackground virtual RefreshBackground
#include <chrome/browser/ui/views/location_bar/location_bar_view.h>  // IWYU pragma: export
#undef RefreshBackground
#undef GetBorderRadius
#undef OnOmniboxBlurred
#undef Init
#undef GetBackgroundColorForTesting

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_LOCATION_BAR_VIEW_H_
