/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/general_browser_usage.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_MAC)
#include "base/mac/mac_util.h"
#endif

namespace misc_metrics {

class GeneralBrowserUsageUnitTest : public testing::Test {
 public:
  GeneralBrowserUsageUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    misc_metrics::GeneralBrowserUsage::RegisterPrefs(local_state_.registry());
    ResetHistogramTester();

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

  void ResetHistogramTester() {
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

 protected:
  void SetUpUsage(std::optional<std::string> day_zero_variant,
                  bool is_first_run,
                  base::Time first_run_time) {
    general_browser_usage_ = std::make_unique<GeneralBrowserUsage>(
        &local_state_, day_zero_variant, is_first_run, first_run_time);
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<GeneralBrowserUsage> general_browser_usage_;
};

TEST_F(GeneralBrowserUsageUnitTest, WeeklyUsage) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  SetUpUsage({}, true, base::Time::Now());

  histogram_tester_->ExpectUniqueSample(kWeeklyUseHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Days(1));
  int last_bucket_count =
      histogram_tester_->GetBucketCount(kWeeklyUseHistogramName, 0);
  EXPECT_GE(last_bucket_count, 1);

  task_environment_.FastForwardBy(base::Days(3));

  EXPECT_GT(histogram_tester_->GetBucketCount(kWeeklyUseHistogramName, 0),
            last_bucket_count);
  histogram_tester_->ExpectBucketCount(kWeeklyUseHistogramName, 7, 0);

  task_environment_.FastForwardBy(base::Days(3));
  EXPECT_GE(histogram_tester_->GetBucketCount(kWeeklyUseHistogramName, 7), 1);
}

#if !BUILDFLAG(IS_ANDROID)
TEST_F(GeneralBrowserUsageUnitTest, ProfileCount) {
  SetUpUsage({}, true, base::Time::Now());

  histogram_tester_->ExpectTotalCount(kProfileCountHistogramName, 0);

  general_browser_usage_->ReportProfileCount(1);

  histogram_tester_->ExpectUniqueSample(kProfileCountHistogramName, 1, 1);

  general_browser_usage_->ReportProfileCount(2);

  histogram_tester_->ExpectBucketCount(kProfileCountHistogramName, 2, 1);
}
#endif  // !BUILDFLAG(IS_ANDROID)

TEST_F(GeneralBrowserUsageUnitTest, InstallTimeVariantSwitch) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  base::Time install_time = base::Time::Now();
  SetUpUsage("B", true, install_time);

  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(15));

  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 1, 16);

  SetUpUsage("A", false, install_time);
  // Ensure histogram name does not change if "day zero" is enabled
  // after install; we only want to report the "day zero on" metric
  // if it was enabled at install time.
  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 1, 16);

  task_environment_.FastForwardBy(base::Days(16));
  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 1, 31);

  ResetHistogramTester();
  // Ensure there are no more reports past 30 days
  task_environment_.FastForwardBy(base::Days(5));

  histogram_tester_->ExpectTotalCount(kDayZeroVariantHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kDayZeroVariantHistogramName, 0);
}

TEST_F(GeneralBrowserUsageUnitTest, InstallTimeBasic) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  base::Time install_time = base::Time::Now();
  SetUpUsage("A", true, install_time);

  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Days(15));

  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 0, 16);

  task_environment_.FastForwardBy(base::Days(15));
  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 0, 31);

  task_environment_.FastForwardBy(base::Days(15));
  histogram_tester_->ExpectUniqueSample(kDayZeroVariantHistogramName, 0, 31);
}

}  // namespace misc_metrics
