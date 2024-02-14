// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/common/pref_names.h"
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
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    task_environment_.AdvanceClock(base::Days(2));
    PrefRegistrySimple* registry = pref_service_.registry();
    BraveNewsController::RegisterProfilePrefs(registry);
    metrics_ = std::make_unique<NewsMetrics>(&pref_service_);
  }

  void TearDown() override { metrics_ = nullptr; }

  PrefService* GetPrefs() { return &pref_service_; }

  int GetWeeklySum(const char* pref_name) {
    WeeklyStorage storage(&pref_service_, pref_name);
    return storage.GetWeeklySum();
  }

  std::unique_ptr<NewsMetrics> metrics_;
  content::BrowserTaskEnvironment task_environment_;
  base::HistogramTester histogram_tester_;

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(BraveNewsP3ATest, TestWeeklySessionCountBasic) {
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 0, 1);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 1, 1);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 1);
  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 2);

  metrics_->RecordAtSessionStart();
  metrics_->RecordAtSessionStart();
  metrics_->RecordAtSessionStart();
  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 3, 4);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsWeeklySessionCount), 7);
}

TEST_F(BraveNewsP3ATest, TestWeeklySessionCountTimeFade) {
  metrics_->RecordAtSessionStart();
  metrics_->RecordAtSessionStart();

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordAtSessionStart();

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 3);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsWeeklySessionCount), 3);

  task_environment_.AdvanceClock(base::Days(3));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 1, 2);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 0, 1);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsWeeklySessionCount), 0);
}

TEST_F(BraveNewsP3ATest, TestWeeklyDisplayAdsViewedCount) {
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 0,
                                      1);

  metrics_->RecordWeeklyDisplayAdsViewedCount(true);
  metrics_->RecordWeeklyDisplayAdsViewedCount(true);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordWeeklyDisplayAdsViewedCount(true);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsWeeklyDisplayAdViewedCount), 3);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordWeeklyDisplayAdsViewedCount(false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 2,
                                      3);

  task_environment_.AdvanceClock(base::Days(3));
  metrics_->RecordWeeklyDisplayAdsViewedCount(false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 1,
                                      2);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordWeeklyDisplayAdsViewedCount(false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 0,
                                      2);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsWeeklyDisplayAdViewedCount), 0);
}

TEST_F(BraveNewsP3ATest, TestWeeklyAddedDirectFeedsCount) {
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 0,
                                      1);

  metrics_->RecordWeeklyAddedDirectFeedsCount(1);
  metrics_->RecordWeeklyAddedDirectFeedsCount(1);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordWeeklyAddedDirectFeedsCount(0);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 2,
                                      2);

  metrics_->RecordWeeklyAddedDirectFeedsCount(1);
  metrics_->RecordWeeklyAddedDirectFeedsCount(1);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsWeeklyAddedDirectFeedsCount), 4);

  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 4,
                                      1);
  metrics_->RecordWeeklyAddedDirectFeedsCount(-1);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 3,
                                      2);

  task_environment_.AdvanceClock(base::Days(6));
  metrics_->RecordWeeklyAddedDirectFeedsCount(0);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 1,
                                      2);

  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsWeeklyAddedDirectFeedsCount), 1);
}

TEST_F(BraveNewsP3ATest, TestDirectFeedsTotal) {
  PrefService* prefs = GetPrefs();
  metrics_->RecordAtInit();

  // Should not report if not a monthly user
  histogram_tester_.ExpectTotalCount(kDirectFeedsTotalHistogramName, 0);

  prefs->SetTime(prefs::kBraveNewsLastSessionTime, base::Time::Now());
  metrics_->RecordAtInit();

  histogram_tester_.ExpectTotalCount(kDirectFeedsTotalHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kDirectFeedsTotalHistogramName, 0, 1);

  ScopedDictPrefUpdate update1(prefs, prefs::kBraveNewsDirectFeeds);
  update1->Set("id1", base::Value::Dict());
  ScopedDictPrefUpdate update2(prefs, prefs::kBraveNewsDirectFeeds);
  update2->Set("id2", base::Value::Dict());

  metrics_->RecordDirectFeedsTotal();
  histogram_tester_.ExpectTotalCount(kDirectFeedsTotalHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kDirectFeedsTotalHistogramName, 2, 1);
}

TEST_F(BraveNewsP3ATest, TestTotalCardsViewed) {
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kTotalCardViewsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 0, 1);

  metrics_->RecordTotalActionCount(ActionType::kCardView, 0);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 0, 2);

  metrics_->RecordTotalActionCount(ActionType::kCardView, 1);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 1, 1);

  metrics_->RecordTotalActionCount(ActionType::kCardView, 14);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 3, 1);

  task_environment_.AdvanceClock(base::Days(4));
  EXPECT_EQ(GetWeeklySum(prefs::kBraveNewsTotalCardViews), 15);

  metrics_->RecordAtSessionStart();
  metrics_->RecordTotalActionCount(ActionType::kCardView, 15);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 4, 1);

  metrics_->RecordAtSessionStart();
  metrics_->RecordTotalActionCount(ActionType::kCardView, 15);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 5, 1);

  task_environment_.AdvanceClock(base::Days(4));

  metrics_->RecordAtSessionStart();
  metrics_->RecordTotalActionCount(ActionType::kCardView, 0);
  histogram_tester_.ExpectBucketCount(kTotalCardViewsHistogramName, 4, 2);
}

TEST_F(BraveNewsP3ATest, TestLastUsageTime) {
  metrics_->RecordAtInit();
  // Should not report if News was never used
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.AdvanceClock(base::Days(7));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(7));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 3, 1);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 2);

  task_environment_.AdvanceClock(base::Days(21));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(7));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 5, 1);

  task_environment_.AdvanceClock(base::Days(33));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 6, 1);

  task_environment_.AdvanceClock(base::Days(90));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 6, 2);
}

TEST_F(BraveNewsP3ATest, TestNewUserReturningFollowingDay) {
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 1);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(1));
  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 1);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 2);

  task_environment_.AdvanceClock(base::Days(5));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

TEST_F(BraveNewsP3ATest, TestNewUserReturningNotFollowingDay) {
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 1);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 2);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(2));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 4, 2);

  task_environment_.AdvanceClock(base::Days(4));
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

TEST_F(BraveNewsP3ATest, TestIsEnabled) {
  PrefService* prefs = GetPrefs();

  // should not record "disabled" if never opted in
  prefs->SetBoolean(prefs::kNewTabPageShowToday, false);
  histogram_tester_.ExpectTotalCount(kIsEnabledHistogramName, 0);

  prefs->SetBoolean(prefs::kBraveNewsOptedIn, true);
  prefs->SetBoolean(prefs::kNewTabPageShowToday, true);
  metrics_->RecordFeatureEnabledChange();
  histogram_tester_.ExpectUniqueSample(kIsEnabledHistogramName, 1, 1);

  prefs->SetBoolean(prefs::kNewTabPageShowToday, false);
  metrics_->RecordFeatureEnabledChange();
  histogram_tester_.ExpectBucketCount(kIsEnabledHistogramName, 0, 1);
}

TEST_F(BraveNewsP3ATest, TestGeneralUsage) {
  metrics_->RecordAtInit();
  histogram_tester_.ExpectTotalCount(kUsageDailyHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kUsageMonthlyHistogramName, 0);

  metrics_->RecordAtSessionStart();
  histogram_tester_.ExpectUniqueSample(kUsageDailyHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kUsageMonthlyHistogramName, 1, 1);
}

}  // namespace p3a
}  // namespace brave_news
