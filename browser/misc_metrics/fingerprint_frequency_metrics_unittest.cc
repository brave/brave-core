/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/fingerprint_frequency_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/misc_metrics/features.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_MAC)
#include "base/mac/mac_util.h"
#endif

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kStartDelay = base::Seconds(30);
constexpr base::TimeDelta kRendererInterval = base::Hours(10);

constexpr char kTotalCountKey[] = "total";
constexpr char kLanguagesKey[] = "navigator_languages";
constexpr char kScreenSizeKey[] = "screenSize";

}  // namespace

class FingerprintFrequencyMetricsUnitTest : public testing::Test {
 public:
  FingerprintFrequencyMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kFingerprintInputMetrics, {{"renderer_interval", "10h"}});
  }

  void SetUp() override {
    FingerprintFrequencyMetrics::RegisterPrefs(local_state_.registry());
    metrics_ =
        std::make_unique<FingerprintFrequencyMetrics>(&local_state_, &profile_);
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

  // Expires the reporting frame and triggers reporting.
  void TriggerReport() {
    local_state_.SetTime(kMiscMetricsFingerprintReportFrameStartTime,
                         base::Time::Now() - base::Days(7));
    metrics_->ReportAllMetrics();
  }

  base::test::ScopedFeatureList feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<FingerprintFrequencyMetrics> metrics_;
};

TEST_F(FingerprintFrequencyMetricsUnitTest, ExecutesAfterStartDelay) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  SetResults(1);

  task_environment_.FastForwardBy(base::Seconds(20));
  EXPECT_EQ(GetTotalCount(), 0);

  task_environment_.FastForwardBy(base::Seconds(10));
  EXPECT_EQ(GetTotalCount(), 1);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, SkipsExecutionWithinInterval) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  local_state_.SetTime(kMiscMetricsFingerprintLastExecutionTime,
                       base::Time::Now());
  SetResults(1);

  task_environment_.FastForwardBy(kStartDelay);
  EXPECT_EQ(GetTotalCount(), 0);

  // Subsequent checks keep skipping until the interval elapses.
  task_environment_.FastForwardBy(kRendererInterval);
  EXPECT_EQ(GetTotalCount(), 1);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, ReportsLanguageChange) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(kStartDelay);
  task_environment_.FastForwardBy(kRendererInterval);
  ASSERT_EQ(GetTotalCount(), 2);

  TriggerReport();

  // The languages never changed, so nothing is reported for them.
  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintPercentHistogramName,
                                     0);
  // The screen size never changed either, but is still reported as 0%.
  histogram_tester_.ExpectUniqueSample(
      kScreenSizeFingerprintPercentHistogramName, 0, 1);

  SetResults(2);
  task_environment_.FastForwardBy(kRendererInterval);
  task_environment_.FastForwardBy(kRendererInterval);
  ASSERT_EQ(GetTotalCount(), 2);

  TriggerReport();

  // 1 of 2 executions changed the languages: 50% -> bucket 3.
  histogram_tester_.ExpectUniqueSample(
      kLanguagesFingerprintPercentHistogramName, 3, 1);
  histogram_tester_.ExpectUniqueSample(
      kScreenSizeFingerprintPercentHistogramName, 0, 2);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, NoReportBeforeFrameExpires) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(kStartDelay);

  SetResults(2);
  task_environment_.FastForwardBy(kRendererInterval);

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintPercentHistogramName,
                                     0);
  histogram_tester_.ExpectTotalCount(kScreenSizeFingerprintPercentHistogramName,
                                     0);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, ReportsMetricsViaReportTimer) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintPercentHistogramName,
                                     0);
  histogram_tester_.ExpectTotalCount(kScreenSizeFingerprintPercentHistogramName,
                                     0);

  task_environment_.FastForwardBy(base::Days(4));

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintPercentHistogramName,
                                     0);
  histogram_tester_.ExpectUniqueSample(
      kScreenSizeFingerprintPercentHistogramName, 0, 1);
}

}  // namespace misc_metrics
