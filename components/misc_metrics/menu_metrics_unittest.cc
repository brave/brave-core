/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class MenuMetricsUnitTest : public testing::Test {
 public:
  MenuMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    misc_metrics::MenuMetrics::RegisterPrefs(local_state_.registry());
    menu_metrics_ = std::make_unique<MenuMetrics>(&local_state_);
  }

 protected:
  void Reset() { menu_metrics_ = std::make_unique<MenuMetrics>(&local_state_); }

  content::BrowserTaskEnvironment task_environment_;
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
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 2);

  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);
  }

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 5);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);
  }

  // "browser views" is the most frequent with a max count of 5
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 6);
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 2, 1);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBraveFeatures);
  }

  // "brave features" is the most frequent with a max count of 6
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 8);
}

TEST_F(MenuMetricsUnitTest, MostFrequentWithReset) {
  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kTabWindow);
  }
  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBraveFeatures);
  }

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 2);

  Reset();

  menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 1, 3);

  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuGroupAction(MenuGroup::kBrowserViews);
  }

  histogram_tester_.ExpectBucketCount(kFrequentMenuGroupHistogramName, 2, 1);
}

TEST_F(MenuMetricsUnitTest, DismissRate) {
  histogram_tester_.ExpectUniqueSample(kMenuDismissRateHistogramName, 0, 1);

  for (size_t i = 0; i < 10; i++) {
    menu_metrics_->RecordMenuShown();
  }

  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 1, 10);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuDismiss();
  }

  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 1, 12);

  menu_metrics_->RecordMenuDismiss();

  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 2, 1);

  for (size_t i = 0; i < 2; i++) {
    menu_metrics_->RecordMenuDismiss();
  }

  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 3, 1);

  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuDismiss();
  }

  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 4, 1);

  task_environment_.FastForwardBy(base::Days(4));

  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 4, 5);

  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 4, 7);
  histogram_tester_.ExpectBucketCount(kMenuDismissRateHistogramName, 0, 2);
}

TEST_F(MenuMetricsUnitTest, OpenCount) {
  histogram_tester_.ExpectUniqueSample(kMenuOpensHistogramName, 0, 1);

  for (size_t i = 0; i < 3; i++) {
    menu_metrics_->RecordMenuShown();
  }

  histogram_tester_.ExpectBucketCount(kMenuOpensHistogramName, 1, 3);

  for (size_t i = 0; i < 4; i++) {
    menu_metrics_->RecordMenuShown();
  }

  histogram_tester_.ExpectBucketCount(kMenuOpensHistogramName, 1, 5);
  histogram_tester_.ExpectBucketCount(kMenuOpensHistogramName, 2, 2);

  task_environment_.FastForwardBy(base::Days(4));

  histogram_tester_.ExpectBucketCount(kMenuOpensHistogramName, 2, 6);
  histogram_tester_.ExpectBucketCount(kMenuOpensHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectBucketCount(kMenuOpensHistogramName, 2, 8);
  histogram_tester_.ExpectBucketCount(kMenuOpensHistogramName, 0, 2);
}

}  // namespace misc_metrics
