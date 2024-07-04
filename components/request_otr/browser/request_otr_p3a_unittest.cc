/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace request_otr::p3a {

class RequestOTRP3AUnitTest : public testing::Test {
 public:
  RequestOTRP3AUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~RequestOTRP3AUnitTest() override = default;
  // testing::Test:
  void SetUp() override {
    RegisterProfilePrefs(profile_prefs_.registry());
    task_environment_.AdvanceClock(base::Days(31));
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple profile_prefs_;
  base::HistogramTester histogram_tester_;
};

TEST_F(RequestOTRP3AUnitTest, InterstitialShown) {
  p3a::RecordInterstitialShown(&profile_prefs_, false);
  histogram_tester_.ExpectUniqueSample(kInterstitialShownHistogramName, 0, 1);

  p3a::RecordInterstitialShown(&profile_prefs_, true);
  histogram_tester_.ExpectBucketCount(kInterstitialShownHistogramName, 1, 1);

  p3a::RecordInterstitialShown(&profile_prefs_, true);
  histogram_tester_.ExpectBucketCount(kInterstitialShownHistogramName, 2, 1);

  UpdateMetrics(&profile_prefs_);
  histogram_tester_.ExpectBucketCount(kInterstitialShownHistogramName, 2, 2);

  task_environment_.FastForwardBy(base::Days(31));
  UpdateMetrics(&profile_prefs_);
  histogram_tester_.ExpectBucketCount(kInterstitialShownHistogramName, 0, 2);
  histogram_tester_.ExpectTotalCount(kInterstitialShownHistogramName, 5);
}

TEST_F(RequestOTRP3AUnitTest, InterstitialDuration) {
  p3a::RecordInterstitialShown(&profile_prefs_, false);
  p3a::RecordInterstitialEnd(&profile_prefs_, {});

  histogram_tester_.ExpectTotalCount(kInterstitialDurationHistogramName, 0);

  base::Time session_start_time = base::Time::Now();
  task_environment_.FastForwardBy(base::Seconds(12));

  p3a::RecordInterstitialShown(&profile_prefs_, true);
  p3a::RecordInterstitialEnd(&profile_prefs_, session_start_time);
  histogram_tester_.ExpectUniqueSample(kInterstitialDurationHistogramName, 2,
                                       1);

  session_start_time = base::Time::Now();
  task_environment_.FastForwardBy(base::Seconds(300));

  p3a::RecordInterstitialShown(&profile_prefs_, true);
  p3a::RecordInterstitialEnd(&profile_prefs_, session_start_time);
  histogram_tester_.ExpectBucketCount(kInterstitialDurationHistogramName, 5, 1);

  UpdateMetrics(&profile_prefs_);
  histogram_tester_.ExpectBucketCount(kInterstitialDurationHistogramName, 5, 2);

  task_environment_.FastForwardBy(base::Days(31));
  UpdateMetrics(&profile_prefs_);

  histogram_tester_.ExpectTotalCount(kInterstitialDurationHistogramName, 3);
}

TEST_F(RequestOTRP3AUnitTest, SessionCount) {
  p3a::RecordSessionCount(&profile_prefs_, {});
  histogram_tester_.ExpectUniqueSample(kSessionCountHistogramName, 0, 1);

  p3a::RecordSessionCount(&profile_prefs_, true);
  histogram_tester_.ExpectBucketCount(kSessionCountHistogramName, 1, 1);

  p3a::RecordSessionCount(&profile_prefs_, true);
  histogram_tester_.ExpectBucketCount(kSessionCountHistogramName, 2, 1);

  p3a::UpdateMetrics(&profile_prefs_);
  histogram_tester_.ExpectBucketCount(kSessionCountHistogramName, 2, 2);

  task_environment_.FastForwardBy(base::Days(31));
  p3a::UpdateMetrics(&profile_prefs_);
  histogram_tester_.ExpectBucketCount(kSessionCountHistogramName, 0, 2);

  histogram_tester_.ExpectTotalCount(kSessionCountHistogramName, 5);
}

}  // namespace request_otr::p3a
