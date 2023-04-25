/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/general_browser_usage.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class GeneralBrowserUsageUnitTest : public testing::Test {
 public:
  GeneralBrowserUsageUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    misc_metrics::GeneralBrowserUsage::RegisterPrefs(local_state_.registry());

    // skip ahead to next monday if not on monday
    base::Time now = base::Time::Now();
    base::Time::Exploded exploded;
    now.LocalMidnight().LocalExplode(&exploded);
    int days_until_monday = 0;
    if (exploded.day_of_week > 1) {
      days_until_monday = 8 - exploded.day_of_week;
    } else if (exploded.day_of_week == 0) {
      days_until_monday = 1;
    }
    task_environment_.AdvanceClock(base::Days(days_until_monday));

    general_browser_usage_ =
        std::make_unique<GeneralBrowserUsage>(&local_state_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<GeneralBrowserUsage> general_browser_usage_;
};

TEST_F(GeneralBrowserUsageUnitTest, WeeklyUsage) {
  histogram_tester_.ExpectUniqueSample(kWeeklyUseHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectUniqueSample(kWeeklyUseHistogramName, 0, 2);

  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectUniqueSample(kWeeklyUseHistogramName, 0, 5);

  task_environment_.FastForwardBy(base::Days(3));
  histogram_tester_.ExpectBucketCount(kWeeklyUseHistogramName, 7, 1);
}

#if !BUILDFLAG(IS_ANDROID)
TEST_F(GeneralBrowserUsageUnitTest, ProfileCount) {
  histogram_tester_.ExpectTotalCount(kProfileCountHistogramName, 0);

  general_browser_usage_->ReportProfileCount(1);

  histogram_tester_.ExpectUniqueSample(kProfileCountHistogramName, 1, 1);

  general_browser_usage_->ReportProfileCount(2);

  histogram_tester_.ExpectBucketCount(kProfileCountHistogramName, 2, 1);
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace misc_metrics
