/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_layout_provider.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace {

#if BUILDFLAG(IS_WIN)
// Debug: `--enable-features=BraveWinLegacyRoundedCornerLayoutMetricsDebug`
BASE_FEATURE(kBraveWinLegacyRoundedCornerLayoutMetricsDebug,
             "BraveWinLegacyRoundedCornerLayoutMetricsDebug",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

}  // namespace

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

int BraveLayoutProvider::GetCornerRadiusMetric(
    views::ShapeContextTokensOverride token) const {
#if BUILDFLAG(IS_WIN)
  if (base::FeatureList::IsEnabled(
          kBraveWinLegacyRoundedCornerLayoutMetricsDebug)) {
    using views::ShapeContextTokensOverride;
    switch (token) {
      case ShapeContextTokensOverride::kRoundedCornersBorderRadius:
      case ShapeContextTokensOverride::kRoundedCornersBorderRadiusAtWindowCorner:
        return 4;
      default:
        break;
    }
  }
#endif
  return LayoutProvider::GetCornerRadiusMetric(token);
}

int BraveLayoutProvider::GetDistanceMetric(int metric) const {
  if (metric == views::DISTANCE_CONTROL_VERTICAL_TEXT_PADDING) {
    return 8;
  }

  return ChromeLayoutProvider::GetDistanceMetric(metric);
}
