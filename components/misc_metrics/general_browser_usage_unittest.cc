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
  }

 protected:
  void SetUpUsage(bool day_zero_experiment_enabled,
                  bool is_first_run,
                  base::Time first_run_time) {
    general_browser_usage_ = std::make_unique<GeneralBrowserUsage>(
        &local_state_, day_zero_experiment_enabled, is_first_run,
        first_run_time);
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<GeneralBrowserUsage> general_browser_usage_;
};

TEST_F(GeneralBrowserUsageUnitTest, WeeklyUsage) {
  SetUpUsage(false, true, base::Time::Now());

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
  SetUpUsage(false, true, base::Time::Now());

  histogram_tester_.ExpectTotalCount(kProfileCountHistogramName, 0);

  general_browser_usage_->ReportProfileCount(1);

  histogram_tester_.ExpectUniqueSample(kProfileCountHistogramName, 1, 1);

  general_browser_usage_->ReportProfileCount(2);

  histogram_tester_.ExpectBucketCount(kProfileCountHistogramName, 2, 1);
}
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_WIN)
TEST_F(GeneralBrowserUsageUnitTest, InstallTimeOff) {
  base::Time install_time = base::Time::Now();
  SetUpUsage(false, true, install_time);

  histogram_tester_.ExpectUniqueSample(kDayZeroOffInstallTime, 0, 1);

  task_environment_.FastForwardBy(base::Days(15));

  histogram_tester_.ExpectBucketCount(kDayZeroOffInstallTime, 15, 1);
  histogram_tester_.ExpectTotalCount(kDayZeroOnInstallTime, 0);

  SetUpUsage(true, false, install_time);
  // Ensure histogram name does not change if "day zero" is enabled
  // after install; we only want to report the "day zero on" metric
  // if it was enabled at install time.
  histogram_tester_.ExpectBucketCount(kDayZeroOffInstallTime, 15, 2);
  histogram_tester_.ExpectTotalCount(kDayZeroOnInstallTime, 0);

  task_environment_.FastForwardBy(base::Days(15));
  histogram_tester_.ExpectBucketCount(kDayZeroOffInstallTime, 30, 1);

  // Ensure there are no more reports past 30 days
  task_environment_.FastForwardBy(base::Days(5));

  histogram_tester_.ExpectTotalCount(kDayZeroOffInstallTime, 32);
  histogram_tester_.ExpectTotalCount(kDayZeroOnInstallTime, 0);
}

TEST_F(GeneralBrowserUsageUnitTest, InstallTimeOn) {
  base::Time install_time = base::Time::Now();
  SetUpUsage(true, true, install_time);

  histogram_tester_.ExpectUniqueSample(kDayZeroOnInstallTime, 0, 1);

  task_environment_.FastForwardBy(base::Days(15));

  histogram_tester_.ExpectBucketCount(kDayZeroOnInstallTime, 15, 1);
  histogram_tester_.ExpectTotalCount(kDayZeroOffInstallTime, 0);

  SetUpUsage(false, false, install_time);
  histogram_tester_.ExpectBucketCount(kDayZeroOnInstallTime, 15, 2);
  histogram_tester_.ExpectTotalCount(kDayZeroOffInstallTime, 0);
}
#endif  // BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_WIN)

}  // namespace misc_metrics
