/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace playlist {

class PlaylistP3AUnitTest : public testing::Test {
 public:
  PlaylistP3AUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~PlaylistP3AUnitTest() override = default;

  // testing::Test:
  void SetUp() override {
    PlaylistServiceFactory::RegisterLocalStatePrefs(local_state_.registry());
    playlist_p3a_ =
        std::make_unique<PlaylistP3A>(&local_state_, base::Time::Now());
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<PlaylistP3A> playlist_p3a_;
};

TEST_F(PlaylistP3AUnitTest, LastUsageTime) {
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);
  task_environment_.FastForwardBy(base::Days(8));
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectUniqueSample(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 3, 1);
}

TEST_F(PlaylistP3AUnitTest, DaysInWeekUsed) {
  histogram_tester_.ExpectTotalCount(kUsageDaysInWeekHistogramName, 0);
  task_environment_.FastForwardBy(base::Days(8));
  histogram_tester_.ExpectTotalCount(kUsageDaysInWeekHistogramName, 0);

  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectUniqueSample(kUsageDaysInWeekHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(2));

  histogram_tester_.ExpectUniqueSample(kUsageDaysInWeekHistogramName, 1, 3);

  playlist_p3a_->ReportNewUsage();
  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectBucketCount(kUsageDaysInWeekHistogramName, 1, 5);

  task_environment_.FastForwardBy(base::Days(1));
  playlist_p3a_->ReportNewUsage();
  histogram_tester_.ExpectBucketCount(kUsageDaysInWeekHistogramName, 2, 1);

  histogram_tester_.ExpectTotalCount(kUsageDaysInWeekHistogramName, 7);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectTotalCount(kUsageDaysInWeekHistogramName, 13);

  task_environment_.FastForwardBy(base::Days(7));

  // no new reports if not active
  histogram_tester_.ExpectTotalCount(kUsageDaysInWeekHistogramName, 13);
}

TEST_F(PlaylistP3AUnitTest, NewUserReturning) {
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 0);
  task_environment_.FastForwardBy(base::Days(8));
  histogram_tester_.ExpectTotalCount(kNewUserReturningHistogramName, 0);

  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 2, 1);
  playlist_p3a_->ReportNewUsage();
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 2, 2);

  task_environment_.FastForwardBy(base::Days(1));
  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 2);
}

TEST_F(PlaylistP3AUnitTest, FirstTimeOffsetFirstWeek) {
  histogram_tester_.ExpectTotalCount(kFirstTimeOffsetHistogramName, 0);
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectTotalCount(kFirstTimeOffsetHistogramName, 0);

  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectUniqueSample(kFirstTimeOffsetHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(7));
  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectUniqueSample(kFirstTimeOffsetHistogramName, 1, 1);
}

TEST_F(PlaylistP3AUnitTest, FirstTimeOffsetThirdWeek) {
  histogram_tester_.ExpectTotalCount(kFirstTimeOffsetHistogramName, 0);
  task_environment_.FastForwardBy(base::Days(17));
  histogram_tester_.ExpectTotalCount(kFirstTimeOffsetHistogramName, 0);

  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectUniqueSample(kFirstTimeOffsetHistogramName, 3, 1);

  task_environment_.FastForwardBy(base::Days(7));
  playlist_p3a_->ReportNewUsage();

  histogram_tester_.ExpectUniqueSample(kFirstTimeOffsetHistogramName, 3, 1);
}

}  // namespace playlist
