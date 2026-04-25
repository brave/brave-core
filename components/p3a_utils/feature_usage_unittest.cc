// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/p3a_utils/feature_usage.h"

#include "base/test/metrics/histogram_tester.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"

namespace p3a_utils {

constexpr char kFirstUsagePrefName[] = "brave.feature_usage.first_use";
constexpr char kLastUsagePrefName[] = "brave.feature_usage.last_use";
constexpr char kUsedSecondDayPrefName[] = "brave.feature_usage.used_second_day";
constexpr char kDaysInMonthPrefName[] = "brave.feature_usage.days_in_month";
constexpr char kDaysInWeekPrefName[] = "brave.feature_usage.days_in_week";

constexpr char kNewUserReturningHistogramName[] =
    "Brave.Feature.NewUserReturning";
constexpr char kDaysInMonthHistogramName[] = "Brave.Feature.DaysInMonth";
constexpr char kDaysInWeekHistogramName[] = "Brave.Feature.DaysInWeek";
constexpr char kLastUsageTimeHistogramName[] = "Brave.Feature.LastUsageTime";

class FeatureUsageTest : public testing::Test {
 public:
  FeatureUsageTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    PrefRegistrySimple* registry = pref_service_.registry();
    RegisterFeatureUsagePrefs(registry, kFirstUsagePrefName, kLastUsagePrefName,
                              kUsedSecondDayPrefName, kDaysInMonthPrefName,
                              kDaysInWeekPrefName);
    task_environment_.AdvanceClock(base::Days(2));
  }

  void RecordFeatureUsage() {
    ::p3a_utils::RecordFeatureUsage(&pref_service_, kFirstUsagePrefName,
                                    kLastUsagePrefName);
  }

  void RecordFeatureNewUserReturning() {
    ::p3a_utils::RecordFeatureNewUserReturning(
        &pref_service_, kFirstUsagePrefName, kLastUsagePrefName,
        kUsedSecondDayPrefName, kNewUserReturningHistogramName);
  }

  void RecordFeatureDaysInMonthUsed(bool is_add) {
    ::p3a_utils::RecordFeatureDaysInMonthUsed(
        &pref_service_, is_add, kLastUsagePrefName, kDaysInMonthPrefName,
        kDaysInMonthHistogramName);
  }

  void RecordFeatureDaysInWeekUsed(bool is_add) {
    ::p3a_utils::RecordFeatureDaysInWeekUsed(
        &pref_service_, is_add, kDaysInWeekPrefName, kDaysInWeekHistogramName);
  }

  void RecordFeatureLastUsageTimeMetric() {
    ::p3a_utils::RecordFeatureLastUsageTimeMetric(
        &pref_service_, kLastUsagePrefName, kLastUsageTimeHistogramName);
  }

  void RecordFeatureLastUsageTimeMetricSingleMonth() {
    ::p3a_utils::RecordFeatureLastUsageTimeMetric(
        &pref_service_, kLastUsagePrefName, kLastUsageTimeHistogramName, true);
  }

  content::BrowserTaskEnvironment task_environment_;
  base::HistogramTester histogram_tester_;

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(FeatureUsageTest, TestLastUsageTime) {
  RecordFeatureLastUsageTimeMetric();
  // Should not report if News was never used
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  RecordFeatureUsage();
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.AdvanceClock(base::Days(7));
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(7));
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 3, 1);

  RecordFeatureUsage();
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 2);

  task_environment_.AdvanceClock(base::Days(21));
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(7));
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 5, 1);

  task_environment_.AdvanceClock(base::Days(33));
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 6, 1);

  task_environment_.AdvanceClock(base::Days(90));
  RecordFeatureLastUsageTimeMetric();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 6, 2);
}

TEST_F(FeatureUsageTest, TestLastUsageTimeSingleMonth) {
  RecordFeatureLastUsageTimeMetricSingleMonth();
  // Should not report if News was never used
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  RecordFeatureUsage();
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.AdvanceClock(base::Days(7));
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(7));
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 3, 1);

  RecordFeatureUsage();
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 2);

  task_environment_.AdvanceClock(base::Days(21));
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(7));
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 4, 2);

  task_environment_.AdvanceClock(base::Days(2));
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 4, 3);

  task_environment_.AdvanceClock(base::Days(1));
  RecordFeatureLastUsageTimeMetricSingleMonth();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 7);
}

TEST_F(FeatureUsageTest, TestDaysInMonthUsedCount) {
  RecordFeatureDaysInMonthUsed(false);
  // Should not report if News was never used
  histogram_tester_.ExpectTotalCount(kDaysInMonthHistogramName, 0);

  RecordFeatureUsage();
  RecordFeatureDaysInMonthUsed(true);

  histogram_tester_.ExpectBucketCount(kDaysInMonthHistogramName, 1, 1);
  task_environment_.AdvanceClock(base::Days(1));

  RecordFeatureDaysInMonthUsed(true);

  histogram_tester_.ExpectBucketCount(kDaysInMonthHistogramName, 2, 1);
  task_environment_.AdvanceClock(base::Days(14));

  RecordFeatureDaysInMonthUsed(true);
  RecordFeatureDaysInMonthUsed(true);
  RecordFeatureDaysInMonthUsed(true);

  task_environment_.AdvanceClock(base::Days(1));

  RecordFeatureDaysInMonthUsed(true);
  RecordFeatureDaysInMonthUsed(true);
  RecordFeatureDaysInMonthUsed(true);

  histogram_tester_.ExpectTotalCount(kDaysInMonthHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kDaysInMonthHistogramName, 3, 6);

  task_environment_.AdvanceClock(base::Days(20));
  RecordFeatureDaysInMonthUsed(false);

  histogram_tester_.ExpectTotalCount(kDaysInMonthHistogramName, 9);
  histogram_tester_.ExpectBucketCount(kDaysInMonthHistogramName, 2, 2);
}

TEST_F(FeatureUsageTest, TestDaysInWeekUsedCount) {
  histogram_tester_.ExpectTotalCount(kDaysInWeekHistogramName, 0);
  RecordFeatureDaysInWeekUsed(true);

  histogram_tester_.ExpectUniqueSample(kDaysInWeekHistogramName, 1, 1);
  // same day
  RecordFeatureDaysInWeekUsed(true);
  histogram_tester_.ExpectUniqueSample(kDaysInWeekHistogramName, 1, 2);
  task_environment_.AdvanceClock(base::Days(1));

  RecordFeatureDaysInWeekUsed(true);
  // second day
  histogram_tester_.ExpectUniqueSample(kDaysInWeekHistogramName, 1, 3);
  task_environment_.AdvanceClock(base::Days(1));

  RecordFeatureDaysInWeekUsed(true);
  histogram_tester_.ExpectBucketCount(kDaysInWeekHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(14));
  RecordFeatureDaysInWeekUsed(true);
  histogram_tester_.ExpectBucketCount(kDaysInWeekHistogramName, 1, 4);
  RecordFeatureDaysInWeekUsed(false);
  RecordFeatureDaysInWeekUsed(false);
  RecordFeatureDaysInWeekUsed(false);
  RecordFeatureDaysInWeekUsed(false);
  histogram_tester_.ExpectBucketCount(kDaysInWeekHistogramName, 1, 8);

  histogram_tester_.ExpectTotalCount(kDaysInWeekHistogramName, 9);
  task_environment_.AdvanceClock(base::Days(14));
  RecordFeatureDaysInWeekUsed(false);
  histogram_tester_.ExpectTotalCount(kDaysInWeekHistogramName, 9);
}

TEST_F(FeatureUsageTest, TestNewUserReturningFollowingDay) {
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 1);

  RecordFeatureUsage();
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(1));
  RecordFeatureUsage();
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 1);

  task_environment_.AdvanceClock(base::Days(2));
  RecordFeatureUsage();
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 2);

  task_environment_.AdvanceClock(base::Days(5));
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

TEST_F(FeatureUsageTest, TestNewUserReturningNotFollowingDay) {
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 1);

  RecordFeatureUsage();
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(2));
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 2);

  RecordFeatureUsage();
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(2));
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 4, 2);

  task_environment_.AdvanceClock(base::Days(4));
  RecordFeatureNewUserReturning();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

}  // namespace p3a_utils
