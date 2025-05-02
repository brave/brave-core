/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_tab_focus_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatTabFocusMetricsUnitTest : public testing::Test,
                                      public AIChatTabFocusMetrics::Delegate {
 public:
  AIChatTabFocusMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    AIChatTabFocusMetrics::RegisterPrefs(local_state_.registry());
    profile_prefs_.registry()->RegisterBooleanPref(
        prefs::kBraveAIChatTabOrganizationEnabled, false);

    metrics_ = std::make_unique<AIChatTabFocusMetrics>(&local_state_,
                                                       &profile_prefs_, this);
  }

  bool IsPremium() const override { return is_premium_; }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<AIChatTabFocusMetrics> metrics_;
  bool is_premium_ = false;
};

TEST_F(AIChatTabFocusMetricsUnitTest, TabCountMetrics) {
  histogram_tester_.ExpectTotalCount(kTabFocusAvgTabCountHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kTabFocusMaxTabCountHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kTabFocusSessionCountHistogramName, 0);

  metrics_->RecordUsage(5);
  histogram_tester_.ExpectUniqueSample(kTabFocusAvgTabCountHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kTabFocusMaxTabCountHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kTabFocusSessionCountHistogramName, 0,
                                       1);

  metrics_->RecordUsage(15);
  histogram_tester_.ExpectBucketCount(kTabFocusAvgTabCountHistogramName, 1, 1);
  histogram_tester_.ExpectBucketCount(kTabFocusMaxTabCountHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kTabFocusSessionCountHistogramName, 1, 1);

  metrics_->RecordUsage(30);
  histogram_tester_.ExpectBucketCount(kTabFocusAvgTabCountHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kTabFocusMaxTabCountHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kTabFocusSessionCountHistogramName, 1, 2);

  // Verify weekly reset
  task_environment_.FastForwardBy(base::Days(7));

  metrics_->ReportAllMetrics();
  histogram_tester_.ExpectTotalCount(kTabFocusAvgTabCountHistogramName, 3);
  histogram_tester_.ExpectTotalCount(kTabFocusMaxTabCountHistogramName, 3);
  histogram_tester_.ExpectTotalCount(kTabFocusSessionCountHistogramName, 3);
}

TEST_F(AIChatTabFocusMetricsUnitTest, EnabledMetric) {
  // Test disabled state
  metrics_->ReportAllMetrics();
  histogram_tester_.ExpectUniqueSample(kTabFocusEnabledHistogramName, 0, 1);

  // Test enabled free state
  profile_prefs_.SetBoolean(prefs::kBraveAIChatTabOrganizationEnabled, true);
  is_premium_ = false;
  histogram_tester_.ExpectBucketCount(kTabFocusEnabledHistogramName, 1, 1);

  // Test enabled premium state
  is_premium_ = true;
  metrics_->ReportAllMetrics();
  histogram_tester_.ExpectBucketCount(kTabFocusEnabledHistogramName, 2, 1);
}

TEST_F(AIChatTabFocusMetricsUnitTest, LastUsageTime) {
  histogram_tester_.ExpectTotalCount(kTabFocusLastUsageTimeHistogramName, 0);
  metrics_->RecordUsage(10);
  histogram_tester_.ExpectUniqueSample(kTabFocusLastUsageTimeHistogramName, 1,
                                       1);

  task_environment_.FastForwardBy(base::Days(7));
  metrics_->ReportAllMetrics();
  histogram_tester_.ExpectBucketCount(kTabFocusLastUsageTimeHistogramName, 2,
                                      1);
  histogram_tester_.ExpectTotalCount(kTabFocusLastUsageTimeHistogramName, 2);
}

}  // namespace ai_chat
