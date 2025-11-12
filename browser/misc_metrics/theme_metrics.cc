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

  // our p3a records dark_mode::BraveDarkModeType whenever theme mode changes.
  // And we use BrowserColorScheme instead of it. It's underlying int value is
  // different with brave dark mode type. For this migration step, color scheme
  // type is converted to existing p3a value(BraveDarkModeType).
  // TODO(https://github.com/brave/brave-browser/issues/50811): Remove type
  // converting.
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
