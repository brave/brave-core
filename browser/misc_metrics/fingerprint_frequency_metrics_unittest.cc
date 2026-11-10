/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/fingerprint_frequency_metrics.h"

#include <memory>
#include <string_view>

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
constexpr base::TimeDelta kRendererInterval = base::Hours(5);

constexpr char kLanguagesKey[] = "navigator_languages";
constexpr char kScreenSizeKey[] = "screenSize";
constexpr char kCombinedKey[] = "combined";

}  // namespace

class FingerprintFrequencyMetricsUnitTest : public testing::Test {
 public:
  FingerprintFrequencyMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kFingerprintInputMetrics, {{"renderer_interval", "5h"}});
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
            .Set(kScreenSizeKey, 100)
            .Set(kCombinedKey, languages_hash + 1000));
  }

  size_t GetUniqueHashCount(std::string_view key) {
    const base::ListValue* list =
        local_state_.GetDict(kMiscMetricsFingerprintHashes).FindList(key);
    return list ? list->size() : 0;
  }

  // Runs one renderer execution per languages hash value, starting from
  // `first_languages_hash`.
  void RunUniqueExecutions(int first_languages_hash, int count) {
    for (int i = 0; i < count; i++) {
      SetResults(first_languages_hash + i);
      task_environment_.FastForwardBy(kRendererInterval);
    }
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
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  SetResults(1);

  task_environment_.FastForwardBy(base::Seconds(20));
  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 0u);

  task_environment_.FastForwardBy(base::Seconds(10));
  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 1u);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, SkipsExecutionWithinInterval) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  local_state_.SetTime(kMiscMetricsFingerprintLastExecutionTime,
                       base::Time::Now());
  SetResults(1);

  task_environment_.FastForwardBy(kStartDelay);
  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 0u);

  task_environment_.FastForwardBy(kRendererInterval);
  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 1u);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, StoresUniqueHashes) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(kStartDelay);
  task_environment_.FastForwardBy(kRendererInterval);
  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 1u);

  SetResults(2);
  task_environment_.FastForwardBy(kRendererInterval);

  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 2u);
  EXPECT_EQ(GetUniqueHashCount(kCombinedKey), 2u);
  EXPECT_EQ(GetUniqueHashCount(kScreenSizeKey), 1u);

  SetResults(1);
  task_environment_.FastForwardBy(kRendererInterval);

  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 2u);
  EXPECT_EQ(GetUniqueHashCount(kCombinedKey), 2u);
  EXPECT_EQ(GetUniqueHashCount(kScreenSizeKey), 1u);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, ReportsOnlyCombinedForSingleHash) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(kStartDelay);
  task_environment_.FastForwardBy(kRendererInterval);

  TriggerReport();

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kScreenSizeFingerprintHistogramName, 0);
  histogram_tester_.ExpectUniqueSample(kCombinedFingerprintHistogramName, 1, 1);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, ReportsBuckets) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(kStartDelay);

  // 2 unique hashes -> bucket 2.
  RunUniqueExecutions(1, 2);
  TriggerReport();
  histogram_tester_.ExpectBucketCount(kLanguagesFingerprintHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kCombinedFingerprintHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(kScreenSizeFingerprintHistogramName, 0);

  // 6 unique hashes -> bucket 3.
  RunUniqueExecutions(1, 6);
  TriggerReport();
  histogram_tester_.ExpectBucketCount(kLanguagesFingerprintHistogramName, 3, 1);

  // 12 unique hashes -> bucket 4.
  RunUniqueExecutions(1, 12);
  TriggerReport();
  histogram_tester_.ExpectBucketCount(kLanguagesFingerprintHistogramName, 4, 1);

  // 16 unique hashes -> bucket 5.
  RunUniqueExecutions(1, 16);
  TriggerReport();
  histogram_tester_.ExpectBucketCount(kLanguagesFingerprintHistogramName, 5, 1);

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintHistogramName, 4);
  histogram_tester_.ExpectTotalCount(kCombinedFingerprintHistogramName, 4);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, ClearsHashesAfterReport) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(kStartDelay);
  RunUniqueExecutions(1, 3);
  ASSERT_EQ(GetUniqueHashCount(kLanguagesKey), 3u);

  TriggerReport();
  EXPECT_EQ(GetUniqueHashCount(kLanguagesKey), 0u);
  EXPECT_EQ(GetUniqueHashCount(kCombinedKey), 0u);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, NoReportBeforeFrameExpires) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(kStartDelay);

  SetResults(2);
  task_environment_.FastForwardBy(kRendererInterval);

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kCombinedFingerprintHistogramName, 0);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, ReportsMetricsViaReportTimer) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  SetResults(1);
  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectTotalCount(kCombinedFingerprintHistogramName, 0);

  task_environment_.FastForwardBy(base::Days(4));

  histogram_tester_.ExpectTotalCount(kLanguagesFingerprintHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kScreenSizeFingerprintHistogramName, 0);
  histogram_tester_.ExpectUniqueSample(kCombinedFingerprintHistogramName, 1, 1);
}

TEST_F(FingerprintFrequencyMetricsUnitTest, MigratesObsoletePrefs) {
  FingerprintFrequencyMetrics::RegisterPrefsForMigration(
      local_state_.registry());
  local_state_.SetDict(kMiscMetricsFingerprintChangeCounts,
                       base::DictValue().Set("total", 3));
  ASSERT_TRUE(local_state_.HasPrefPath(kMiscMetricsFingerprintChangeCounts));

  FingerprintFrequencyMetrics::MigrateObsoletePrefs(&local_state_);
  EXPECT_FALSE(local_state_.HasPrefPath(kMiscMetricsFingerprintChangeCounts));
}

}  // namespace misc_metrics
