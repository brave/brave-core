/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/constants/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

class BraveVpnMetricsTest : public testing::Test {
 public:
  BraveVpnMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    base::Time future_mock_time;
    if (base::Time::FromString("2023-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }
    profile_prefs_.registry()->RegisterBooleanPref(kNewTabPageShowBraveVPN,
                                                   true);
    brave_vpn::RegisterLocalStatePrefs(local_state_.registry());
    metrics_ =
        std::make_unique<BraveVpnMetrics>(&local_state_, &profile_prefs_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  std::unique_ptr<BraveVpnMetrics> metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(BraveVpnMetricsTest, NewUserReturningMetric) {
  metrics_->RecordAllMetrics(false);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 2);

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->RecordAllMetrics(true);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->RecordAllMetrics(true);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 1);

  task_environment_.FastForwardBy(base::Days(6));
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

TEST_F(BraveVpnMetricsTest, DaysInMonthUsedMetric) {
  metrics_->RecordAllMetrics(false);
  histogram_tester_.ExpectTotalCount(kDaysInMonthUsedHistogramName, 0);

  metrics_->RecordAllMetrics(true);
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->RecordAllMetrics(true);
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 2, 1);
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 2, 2);

  metrics_->RecordAllMetrics(true);
  task_environment_.FastForwardBy(base::Days(30));
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 0, 1);
}

TEST_F(BraveVpnMetricsTest, LastUsageTimeMetric) {
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  metrics_->RecordAllMetrics(true);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.AdvanceClock(base::Days(10));
  metrics_->RecordAllMetrics(true);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 2);
  task_environment_.AdvanceClock(base::Days(10));
  metrics_->RecordAllMetrics(false);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 2, 1);
}

TEST_F(BraveVpnMetricsTest, WidgetUsageAndHideMetrics) {
  histogram_tester_.ExpectTotalCount(kWidgetUsageHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kHideWidgetHistogramName, 0);

  // Test widget usage recording
  metrics_->RecordWidgetUsage(true);
  histogram_tester_.ExpectUniqueSample(kWidgetUsageHistogramName, 0, 1);

  // Record multiple usages
  metrics_->RecordWidgetUsage(true);
  metrics_->RecordWidgetUsage(true);
  histogram_tester_.ExpectBucketCount(kWidgetUsageHistogramName, 1, 2);

  profile_prefs_.SetBoolean(kNewTabPageShowBraveVPN, true);
  histogram_tester_.ExpectTotalCount(kHideWidgetHistogramName, 0);
  // Test hide widget metric
  profile_prefs_.SetBoolean(kNewTabPageShowBraveVPN, false);
  histogram_tester_.ExpectUniqueSample(kHideWidgetHistogramName, true, 1);

  histogram_tester_.ExpectTotalCount(kWidgetUsageHistogramName, 3);
}

}  // namespace brave_vpn
