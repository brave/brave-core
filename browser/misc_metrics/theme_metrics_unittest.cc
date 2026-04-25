// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/misc_metrics/theme_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/mojom/themes.mojom-shared.h"

namespace misc_metrics {

class ThemeMetricsTest : public testing::Test {
 public:
  void SetUp() override {
    theme_service_ = ThemeServiceFactory::GetForProfile(&profile_);
    theme_metrics_ = std::make_unique<ThemeMetrics>(theme_service_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  raw_ptr<ThemeService> theme_service_;
  std::unique_ptr<ThemeMetrics> theme_metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(ThemeMetricsTest, ReportMetrics) {
  histogram_tester_.ExpectUniqueSample(kBrowserColorSchemeHistogramName, 0, 1);
  theme_service_->SetBrowserColorScheme(
      ThemeService::BrowserColorScheme::kDark);
  histogram_tester_.ExpectBucketCount(kBrowserColorSchemeHistogramName, 1, 1);

  theme_service_->SetBrowserColorScheme(
      ThemeService::BrowserColorScheme::kLight);
  histogram_tester_.ExpectBucketCount(kBrowserColorSchemeHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(kBrowserColorSchemeHistogramName, 3);

  histogram_tester_.ExpectUniqueSample(kThemeColorDefaultHistogramName, 1, 3);
}

}  // namespace misc_metrics
