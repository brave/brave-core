// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/common/pref_names.h"
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
    PrefRegistrySimple* registry = pref_service_.registry();
    BraveNewsController::RegisterProfilePrefs(registry);
    task_environment_.AdvanceClock(base::Days(2));
  }

  PrefService* GetPrefs() { return &pref_service_; }

  content::BrowserTaskEnvironment task_environment_;
  base::HistogramTester histogram_tester_;

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(BraveNewsP3ATest, TestEverInteracted) {
  RecordEverInteracted();
  histogram_tester_.ExpectTotalCount(kEverInteractedHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kEverInteractedHistogramName, 1, 1);
}

TEST_F(BraveNewsP3ATest, TestWeeklySessionCountBasic) {
  PrefService* prefs = GetPrefs();

  RecordAtStart(prefs);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 0, 1);

  RecordWeeklySessionCount(prefs, true);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 1, 1);

  RecordWeeklySessionCount(prefs, true);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 1);
  RecordWeeklySessionCount(prefs, true);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 2);

  RecordWeeklySessionCount(prefs, true);
  RecordWeeklySessionCount(prefs, true);
  RecordWeeklySessionCount(prefs, true);
  RecordWeeklySessionCount(prefs, true);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 3, 4);
}

TEST_F(BraveNewsP3ATest, TestWeeklySessionCountTimeFade) {
  PrefService* prefs = GetPrefs();
  RecordWeeklySessionCount(prefs, true);
  RecordWeeklySessionCount(prefs, true);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklySessionCount(prefs, true);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklySessionCount(prefs, false);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 2, 3);

  task_environment_.AdvanceClock(base::Days(3));
  RecordWeeklySessionCount(prefs, false);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 1, 2);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklySessionCount(prefs, false);
  histogram_tester_.ExpectTotalCount(kWeeklySessionCountHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklySessionCountHistogramName, 0, 1);
}

TEST_F(BraveNewsP3ATest, TestWeeklyMaxCardVisitsCount) {
  PrefService* prefs = GetPrefs();
  RecordAtStart(prefs);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 0, 1);

  RecordWeeklyMaxCardVisitsCount(prefs, 14);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 5, 1);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklyMaxCardVisitsCount(prefs, 5);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 5, 2);

  task_environment_.AdvanceClock(base::Days(5));
  RecordWeeklyMaxCardVisitsCount(prefs, 0);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardVisitsHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardVisitsHistogramName, 3, 1);
}

TEST_F(BraveNewsP3ATest, TestWeeklyMaxCardViewsCount) {
  PrefService* prefs = GetPrefs();
  RecordAtStart(prefs);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 0, 1);

  RecordWeeklyMaxCardViewsCount(prefs, 14);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 4, 1);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklyMaxCardViewsCount(prefs, 4);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 4, 2);

  task_environment_.AdvanceClock(base::Days(5));
  RecordWeeklyMaxCardViewsCount(prefs, 0);
  histogram_tester_.ExpectTotalCount(kWeeklyMaxCardViewsHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklyMaxCardViewsHistogramName, 2, 1);
}

TEST_F(BraveNewsP3ATest, TestWeeklyDisplayAdsViewedCount) {
  PrefService* prefs = GetPrefs();
  RecordAtStart(prefs);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 0,
                                      1);

  RecordWeeklyDisplayAdsViewedCount(prefs, true);
  RecordWeeklyDisplayAdsViewedCount(prefs, true);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklyDisplayAdsViewedCount(prefs, true);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklyDisplayAdsViewedCount(prefs, false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 5);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 2,
                                      3);

  task_environment_.AdvanceClock(base::Days(3));
  RecordWeeklyDisplayAdsViewedCount(prefs, false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 1,
                                      2);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklyDisplayAdsViewedCount(prefs, false);
  histogram_tester_.ExpectTotalCount(kWeeklyDisplayAdsViewedHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kWeeklyDisplayAdsViewedHistogramName, 0,
                                      2);
}

TEST_F(BraveNewsP3ATest, TestWeeklyAddedDirectFeedsCount) {
  PrefService* prefs = GetPrefs();
  RecordAtStart(prefs);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 0,
                                      1);

  RecordWeeklyAddedDirectFeedsCount(prefs, 1);
  RecordWeeklyAddedDirectFeedsCount(prefs, 1);

  task_environment_.AdvanceClock(base::Days(2));
  RecordWeeklyAddedDirectFeedsCount(prefs, 0);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 4);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 2,
                                      2);

  RecordWeeklyAddedDirectFeedsCount(prefs, 1);
  RecordWeeklyAddedDirectFeedsCount(prefs, 1);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 6);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 4,
                                      1);
  RecordWeeklyAddedDirectFeedsCount(prefs, -1);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 7);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 3,
                                      2);

  task_environment_.AdvanceClock(base::Days(6));
  RecordWeeklyAddedDirectFeedsCount(prefs, 0);
  histogram_tester_.ExpectTotalCount(kWeeklyAddedDirectFeedsHistogramName, 8);
  histogram_tester_.ExpectBucketCount(kWeeklyAddedDirectFeedsHistogramName, 1,
                                      2);
}

TEST_F(BraveNewsP3ATest, TestDirectFeedsTotal) {
  PrefService* prefs = GetPrefs();
  RecordAtStart(prefs);
  histogram_tester_.ExpectTotalCount(kDirectFeedsTotalHistogramName, 1);
  histogram_tester_.ExpectBucketCount(kDirectFeedsTotalHistogramName, 0, 1);

  DictionaryPrefUpdate update1(prefs, prefs::kBraveTodayDirectFeeds);
  update1->SetPath("id1", base::Value(base::Value::Type::DICTIONARY));
  DictionaryPrefUpdate update2(prefs, prefs::kBraveTodayDirectFeeds);
  update2->SetPath("id2", base::Value(base::Value::Type::DICTIONARY));

  RecordDirectFeedsTotal(prefs);
  histogram_tester_.ExpectTotalCount(kDirectFeedsTotalHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kDirectFeedsTotalHistogramName, 2, 1);
}

}  // namespace p3a
}  // namespace brave_news
