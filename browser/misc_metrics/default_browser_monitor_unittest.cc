/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/default_browser_monitor.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class DefaultBrowserMonitorUnitTest : public ::testing::Test {
 public:
  DefaultBrowserMonitorUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void CreateMonitor() {
    monitor_ = std::make_unique<DefaultBrowserMonitor>(&local_state_);
    monitor_->SetGetDefaultBrowserCallbackForTesting(base::BindRepeating(
        &DefaultBrowserMonitorUnitTest::GetMockedDefaultBrowser,
        base::Unretained(this)));
    monitor_->Start();
    task_environment_.FastForwardBy(base::Minutes(5));
  }

  bool GetMockedDefaultBrowser() { return mocked_is_default_; }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<DefaultBrowserMonitor> monitor_;
  bool mocked_is_default_ = false;
};

TEST_F(DefaultBrowserMonitorUnitTest, ReportsIsNotDefaultState) {
  mocked_is_default_ = false;
  CreateMonitor();

  histogram_tester_.ExpectUniqueSample(kDefaultBrowserHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kDefaultBrowserDailyHistogramName,
                                       INT_MAX - 1, 1);
}

TEST_F(DefaultBrowserMonitorUnitTest, ReportsIsDefaultState) {
  mocked_is_default_ = true;
  CreateMonitor();

  histogram_tester_.ExpectUniqueSample(kDefaultBrowserHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kDefaultBrowserDailyHistogramName, 1, 1);
}

TEST_F(DefaultBrowserMonitorUnitTest, SwitchingBetweenNoAndYesStates) {
  // Start with "no" state (false)
  mocked_is_default_ = false;
  CreateMonitor();

  histogram_tester_.ExpectUniqueSample(kDefaultBrowserHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kDefaultBrowserDailyHistogramName,
                                       INT_MAX - 1, 1);

  // Switch to "yes" state (true) and fast forward to trigger next check
  mocked_is_default_ = true;
  task_environment_.FastForwardBy(base::Hours(4));

  histogram_tester_.ExpectBucketCount(kDefaultBrowserHistogramName, 1, 1);
  histogram_tester_.ExpectBucketCount(kDefaultBrowserDailyHistogramName, 1, 1);

  // Switch back to "no" state (false) and fast forward again
  mocked_is_default_ = false;
  task_environment_.FastForwardBy(base::Hours(4));

  histogram_tester_.ExpectBucketCount(kDefaultBrowserHistogramName, 0, 2);
  histogram_tester_.ExpectBucketCount(kDefaultBrowserDailyHistogramName,
                                      INT_MAX - 1, 2);

  histogram_tester_.ExpectTotalCount(kDefaultBrowserHistogramName, 3);
  histogram_tester_.ExpectTotalCount(kDefaultBrowserDailyHistogramName, 3);
}

}  // namespace misc_metrics
