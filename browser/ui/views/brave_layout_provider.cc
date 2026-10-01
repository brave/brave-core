/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_layout_provider.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#include "base/mac/mac_util.h"
#endif

// static
std::unique_ptr<views::LayoutProvider>
ChromeLayoutProvider::CreateLayoutProvider() {
  return std::make_unique<BraveLayoutProvider>();
}

int BraveLayoutProvider::GetCornerRadiusMetric(views::Emphasis emphasis,
                                               const gfx::Size& size) const {
  switch (emphasis) {
    case views::Emphasis::kNone:
      return 0;
    case views::Emphasis::kLow:
      return 2;
    case views::Emphasis::kMedium:
    case views::Emphasis::kMaximum:
      return 8;
    case views::Emphasis::kHigh:
      return 4;
  }
}

int BraveLayoutProvider::GetCornerRadiusMetric(views::ShapeContextToken token,
                                               const gfx::Size& size) const {
  switch (token) {
    case kBraveOmniboxExpandedRadius:
      return 4;
    case kRoundedCornersBorderRadius:
    case kSidePanelContentRadius:
      // Matches Brave Mac window / content inner rounding.
      return 6;
    case kRoundedCornersBorderRadiusAtWindowCorner:
#if BUILDFLAG(IS_MAC)
      if (base::mac::MacOSMajorVersion() >= 27) {
        return 12;
      }
      if (base::mac::MacOSMajorVersion() >= 26) {
        return 16;
      }
#endif
      return 6;
    default:
      return ChromeLayoutProvider::GetCornerRadiusMetric(token, size);
  }
}

int BraveLayoutProvider::GetDistanceMetric(int metric) const {
  if (metric == views::DISTANCE_CONTROL_VERTICAL_TEXT_PADDING) {
    return 8;
  }

  return ChromeLayoutProvider::GetDistanceMetric(metric);
}
