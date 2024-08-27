// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/misc_metrics/theme_metrics.h"

#include "base/check_is_test.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"

namespace misc_metrics {

ThemeMetrics::ThemeMetrics(ThemeService* theme_service)
    : theme_service_(theme_service) {
  if (g_browser_process->local_state()) {
    pref_change_registrar_.Init(g_browser_process->local_state());
    pref_change_registrar_.Add(kBraveDarkMode,
                               base::BindRepeating(&ThemeMetrics::ReportMetrics,
                                                   base::Unretained(this)));
  } else {
    CHECK_IS_TEST();
  }
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
      static_cast<int>(dark_mode::GetBraveDarkModeType()), 3);
  UMA_HISTOGRAM_BOOLEAN(kThemeColorDefaultHistogramName,
                        theme_service_->UsingDefaultTheme());
}

void ThemeMetrics::OnThemeChanged() {
  ReportMetrics();
}

}  // namespace misc_metrics
