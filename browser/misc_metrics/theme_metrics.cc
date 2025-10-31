// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/misc_metrics/theme_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "chrome/browser/themes/theme_service.h"

namespace {

dark_mode::BraveDarkModeType GetBraveDarkModeType(ThemeService* service) {
  auto type = service->GetBrowserColorScheme();
  if (type == ThemeService::BrowserColorScheme::kSystem) {
    return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT;
  } else if (type == ThemeService::BrowserColorScheme::kLight) {
    return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  } else {
    CHECK_EQ(ThemeService::BrowserColorScheme::kDark, type);
    return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  }
}

}  // namespace

namespace misc_metrics {

ThemeMetrics::ThemeMetrics(ThemeService* theme_service)
    : theme_service_(theme_service) {
  theme_observer_.Observe(theme_service_);
  ReportMetrics();
}

ThemeMetrics::~ThemeMetrics() = default;

void ThemeMetrics::ReportMetrics() {
  if (!theme_service_) {
    return;
  }

  UMA_HISTOGRAM_EXACT_LINEAR(
      kBrowserColorSchemeHistogramName,
      static_cast<int>(GetBraveDarkModeType(theme_service_)), 3);
  UMA_HISTOGRAM_BOOLEAN(kThemeColorDefaultHistogramName,
                        theme_service_->UsingDefaultTheme());
}

void ThemeMetrics::OnThemeChanged() {
  ReportMetrics();
}

}  // namespace misc_metrics
