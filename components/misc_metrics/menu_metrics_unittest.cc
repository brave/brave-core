/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class MenuMetricsUnitTest : public testing::Test {
 public:
  void SetUp() override {
    misc_metrics::MenuMetrics::RegisterPrefs(local_state_.registry());
    menu_metrics_ = std::make_unique<MenuMetrics>(&local_state_);
  }

 protected:
  void Reset() { menu_metrics_ = std::make_unique<MenuMetrics>(&local_state_); }

  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<MenuMetrics> menu_metrics_;
};

TEST_F(MenuMetricsUnitTest, MostFrequent) {
  histogram_tester_.ExpectTotalCount(kFrequentMenuGroupHistogramName, 0);

  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kTabWindow);
  }

  // "tab window" is the most frequent with a max count of 3
  histogram_tester_.ExpectUniqueSample(kFrequentMenuGroupHistogramName, 0, 3);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBraveFeatures);
  }

  histogram_tester_.ExpectUniqueSample(kFrequentMenuGroupHistogramName, 0, 5);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBraveFeatures);
  }

  // "brave features" is the most frequent with a max count of 4
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 1);

  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);
  }

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 4);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);
  }

  // "browser views" is the most frequent with a max count of 5
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 5);
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 2, 1);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBraveFeatures);
  }

  // "browser views" is the most frequent with a max count of 6
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 7);
}

TEST_F(MenuMetricsUnitTest, MostFrequentWithReset) {
  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kTabWindow);
  }
  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBraveFeatures);
  }

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 1);

  Reset();

  menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 2);

  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);
  }

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 2, 1);
}

}  // namespace misc_metrics
