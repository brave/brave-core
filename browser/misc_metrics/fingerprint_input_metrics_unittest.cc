/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/fingerprint_input_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

namespace {

// Covers the startup delay, including the maximum random jitter.
constexpr base::TimeDelta kMaxStartDelay = base::Seconds(45);
// Covers the default renderer interval, including the maximum random jitter.
constexpr base::TimeDelta kMaxRendererInterval = base::Hours(11);

constexpr char kTotalCountKey[] = "total";
constexpr char kLanguagesKey[] = "navigator_languages";
constexpr char kScreenSizeKey[] = "screenSize";

}  // namespace

class FingerprintInputMetricsUnitTest : public testing::Test {
 public:
  FingerprintInputMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    FingerprintInputMetrics::RegisterPrefs(local_state_.registry());
    metrics_ =
        std::make_unique<FingerprintInputMetrics>(&local_state_, &profile_);
  }

 protected:
  void SetResults(int languages_hash) {
    metrics_->SetFakeRendererResultsForTesting(
        base::DictValue()
            .Set(kLanguagesKey, languages_hash)
            .Set(kScreenSizeKey, 100));
  }

  int GetTotalCount() {
    return local_state_.GetDict(kMiscMetricsFingerprintChangeCounts)
        .FindInt(kTotalCountKey)
        .value_or(0);
  }

  // Expires the reporting frame and waits for the report timer to observe it.
  void TriggerReport() {
    local_state_.SetTime(kMiscMetricsFingerprintReportFrameStartTime,
                         base::Time::Now() - base::Days(7));
    task_environment_.FastForwardBy(base::Minutes(31));
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<FingerprintInputMetrics> metrics_;
};

TEST_F(FingerprintInputMetricsUnitTest, ExecutesAfterStartDelay) {
  SetResults(1);

  task_environment_.FastForwardBy(base::Seconds(20));
  EXPECT_EQ(GetTotalCount(), 0);

  task_environment_.FastForwardBy(kMaxStartDelay);
  EXPECT_EQ(GetTotalCount(), 1);
}

TEST_F(FingerprintInputMetricsUnitTest, SkipsExecutionWithinInterval) {
  local_state_.SetTime(kMiscMetricsFingerprintLastExecutionTime,
                       base::Time::Now());
  SetResults(1);

  task_environment_.FastForwardBy(kMaxStartDelay);
  EXPECT_EQ(GetTotalCount(), 0);

  // Subsequent checks keep skipping until the interval elapses.
  task_environment_.FastForwardBy(kMaxRendererInterval);
  EXPECT_EQ(GetTotalCount(), 1);
}

TEST_F(FingerprintInputMetricsUnitTest, ReportsLanguageChange) {
  SetResults(1);
  task_environment_.FastForwardBy(kMaxStartDelay);
  task_environment_.FastForwardBy(kMaxRendererInterval);
  ASSERT_EQ(GetTotalCount(), 2);

  TriggerReport();

  // The languages never changed, so nothing is reported for them.
  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintPercentHistogramName,
                                     0);
  // The screen size never changed either, but is still reported as 0%.
  histogram_tester_.ExpectUniqueSample(
      kScreenSizeFingerprintPercentHistogramName, 0, 1);

  SetResults(2);
  task_environment_.FastForwardBy(kMaxRendererInterval);
  task_environment_.FastForwardBy(kMaxRendererInterval);
  ASSERT_EQ(GetTotalCount(), 2);

  TriggerReport();

  // 1 of 2 executions changed the languages: 50% -> bucket 3.
  histogram_tester_.ExpectUniqueSample(
      kLanguagesFingerprintPercentHistogramName, 3, 1);
  histogram_tester_.ExpectUniqueSample(
      kScreenSizeFingerprintPercentHistogramName, 0, 2);
}

TEST_F(FingerprintInputMetricsUnitTest, NoReportBeforeFrameExpires) {
  SetResults(1);
  task_environment_.FastForwardBy(kMaxStartDelay);

  SetResults(2);
  task_environment_.FastForwardBy(kMaxRendererInterval);

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintPercentHistogramName,
                                     0);
  histogram_tester_.ExpectTotalCount(kScreenSizeFingerprintPercentHistogramName,
                                     0);
}

}  // namespace misc_metrics
