// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {
namespace p3a {

class BraveNewsP3ATest : public testing::Test {
 public:
  BraveNewsP3ATest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        news_p3a_(&pref_service_) {}

 protected:
  void SetUp() override {
    PrefRegistrySimple* registry = pref_service_.registry();
    BraveNewsController::RegisterProfilePrefs(registry);
    task_environment_.AdvanceClock(base::Days(2));
  }

  int GetWeeklySum(const char* pref_name) {
    WeeklyStorage storage(&pref_service_, pref_name);
    return storage.GetWeeklySum();
  }

  content::BrowserTaskEnvironment task_environment_;
  base::HistogramTester histogram_tester_;

  NewsP3A news_p3a_;

  TestingPrefServiceSimple pref_service_;
};

TEST_F(BraveNewsP3ATest, TestWeeklySessionCountBasic) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 0, 1);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 1, 1);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 1);
  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 2);

  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 3, 4);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayWeeklySessionCount), 7);
}

TEST_F(BraveNewsP3ATest, TestWeeklySessionCountTimeFade) {
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordAtSessionStart();

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 3);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayWeeklySessionCount), 3);

  task_environment_.AdvanceClock(base::Days(3));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 1, 2);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 0, 1);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayWeeklySessionCount), 0);
}

TEST_F(BraveNewsP3ATest, TestWeeklyMaxCardVisitsCount) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 0, 1);

  news_p3a_.RecordWeeklyMaxCardVisitsCount(14);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 5, 1);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordWeeklyMaxCardVisitsCount(5);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 5, 2);

  task_environment_.AdvanceClock(base::Days(5));
  news_p3a_.RecordWeeklyMaxCardVisitsCount(0);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 3, 1);
}

TEST_F(BraveNewsP3ATest, TestWeeklyMaxCardViewsCount) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 0, 1);

  news_p3a_.RecordCardViewMetrics(5);
  task_environment_.FastForwardBy(base::Seconds(1));
  news_p3a_.RecordCardViewMetrics(10);
  task_environment_.FastForwardBy(base::Seconds(1));
  news_p3a_.RecordCardViewMetrics(14);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordCardViewMetrics(4);
  task_environment_.FastForwardBy(base::Seconds(2));
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 4, 2);

  task_environment_.AdvanceClock(base::Days(5));
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordCardViewMetrics(0);
  task_environment_.FastForwardBy(base::Seconds(2));
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 2, 1);
}

TEST_F(BraveNewsP3ATest, TestWeeklyDisplayAdsViewedCount) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 0,
                                      1);

  news_p3a_.RecordWeeklyDisplayAdsViewedCount(true);
  news_p3a_.RecordWeeklyDisplayAdsViewedCount(true);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordWeeklyDisplayAdsViewedCount(true);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayWeeklyDisplayAdViewedCount), 3);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordWeeklyDisplayAdsViewedCount(false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 2,
                                      3);

  task_environment_.AdvanceClock(base::Days(3));
  news_p3a_.RecordWeeklyDisplayAdsViewedCount(false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 1,
                                      2);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordWeeklyDisplayAdsViewedCount(false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 0,
                                      2);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayWeeklyDisplayAdViewedCount), 0);
}

TEST_F(BraveNewsP3ATest, TestWeeklyAddedDirectFeedsCount) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 0,
                                      1);

  news_p3a_.RecordWeeklyAddedDirectFeedsCount(1);
  news_p3a_.RecordWeeklyAddedDirectFeedsCount(1);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordWeeklyAddedDirectFeedsCount(0);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 2,
                                      2);

  news_p3a_.RecordWeeklyAddedDirectFeedsCount(1);
  news_p3a_.RecordWeeklyAddedDirectFeedsCount(1);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayWeeklyAddedDirectFeedsCount), 4);

  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 4,
                                      1);
  news_p3a_.RecordWeeklyAddedDirectFeedsCount(-1);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 3,
                                      2);

  task_environment_.AdvanceClock(base::Days(6));
  news_p3a_.RecordWeeklyAddedDirectFeedsCount(0);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 1,
                                      2);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayWeeklyAddedDirectFeedsCount), 1);
}

TEST_F(BraveNewsP3ATest, TestDirectFeedsTotal) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kDirectFeedsTotalHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kDirectFeedsTotalHistogramName, 0, 1);

  ScopedDictPrefUpdate update1(&pref_service_, prefs::kBraveTodayDirectFeeds);
  update1->Set("id1", base::Value::Dict());
  ScopedDictPrefUpdate update2(&pref_service_, prefs::kBraveTodayDirectFeeds);
  update2->Set("id2", base::Value::Dict());

  news_p3a_.RecordDirectFeedsTotal();
  histogram_tester_.ExpectTotalCount(kDirectFeedsTotalHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kDirectFeedsTotalHistogramName, 2, 1);
}

TEST_F(BraveNewsP3ATest, TestTotalCardsViewed) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kTotalCardViewsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 0, 1);

  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordCardViewMetrics(0);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 0, 2);

  news_p3a_.RecordCardViewMetrics(1);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 1, 1);

  news_p3a_.RecordCardViewMetrics(6);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 2, 1);

  news_p3a_.RecordCardViewMetrics(11);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 3, 1);

  news_p3a_.RecordCardViewMetrics(15);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 3, 2);

  task_environment_.AdvanceClock(base::Days(4));
  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayTotalCardViews), 15);

  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordCardViewMetrics(5);
  task_environment_.FastForwardBy(base::Seconds(1));
  news_p3a_.RecordCardViewMetrics(10);
  task_environment_.FastForwardBy(base::Seconds(1));
  news_p3a_.RecordCardViewMetrics(15);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 4, 2);

  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordCardViewMetrics(5);
  task_environment_.FastForwardBy(base::Seconds(1));
  news_p3a_.RecordCardViewMetrics(10);
  task_environment_.FastForwardBy(base::Seconds(1));
  news_p3a_.RecordCardViewMetrics(11);
  task_environment_.FastForwardBy(base::Seconds(1));
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 5, 1);

  task_environment_.AdvanceClock(base::Days(4));

  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordCardViewMetrics(0);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 4, 4);
  EXPECT_EQ(GetWeeklySum(prefs::kBraveTodayTotalCardViews), 26);
}

TEST_F(BraveNewsP3ATest, TestLastUsageTime) {
  news_p3a_.RecordAtInit();
  // Should not report if News was never used
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.AdvanceClock(base::Days(7));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(7));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 3, 1);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 2);

  task_environment_.AdvanceClock(base::Days(21));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(7));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 5, 1);

  task_environment_.AdvanceClock(base::Days(33));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 6, 1);

  task_environment_.AdvanceClock(base::Days(90));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 6, 2);
}

TEST_F(BraveNewsP3ATest, TestDaysInMonthUsedCount) {
  news_p3a_.RecordAtInit();
  // Should not report if News was never used
  histogram_tester_.ExpectTotalCount(kDaysInMonthUsedCountHistogramName, 0);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedCountHistogramName, 1, 1);
  task_environment_.AdvanceClock(base::Days(1));
  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedCountHistogramName, 2, 1);
  task_environment_.AdvanceClock(base::Days(14));
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();
  task_environment_.AdvanceClock(base::Days(1));
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();
  news_p3a_.RecordAtSessionStart();

  histogram_tester_.ExpectTotalCount(kDaysInMonthUsedCountHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedCountHistogramName, 3, 6);

  task_environment_.AdvanceClock(base::Days(20));
  news_p3a_.RecordAtInit();

  histogram_tester_.ExpectTotalCount(kDaysInMonthUsedCountHistogramName, 9);
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedCountHistogramName, 2, 2);
}

TEST_F(BraveNewsP3ATest, TestNewUserReturningFollowingDay) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 1);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(1));
  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 1);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 2);

  task_environment_.AdvanceClock(base::Days(5));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

TEST_F(BraveNewsP3ATest, TestNewUserReturningNotFollowingDay) {
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 1);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 2);

  news_p3a_.RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(2));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 4, 2);

  task_environment_.AdvanceClock(base::Days(4));
  news_p3a_.RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

}  // namespace p3a
}  // namespace brave_news
