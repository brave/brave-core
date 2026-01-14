/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/default_browser_monitor.h"

#include <memory>

#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class DefaultBrowserMonitorUnitTest : public ::testing::Test {
 public:
  DefaultBrowserMonitorUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void CreateMonitor() {
#if BUILDFLAG(IS_ANDROID)
    monitor_ = std::make_unique<DefaultBrowserMonitor>();
    monitor_->OnDefaultBrowserStateReceived(mocked_is_default_);
#else
    monitor_ = std::make_unique<DefaultBrowserMonitor>(
        base::BindRepeating(
            &DefaultBrowserMonitorUnitTest::GetMockedDefaultBrowser,
            base::Unretained(this)),
        base::BindLambdaForTesting([]() { return false; }));
    monitor_->Start();
    task_environment_.FastForwardBy(base::Minutes(5));
#endif
  }

  bool GetMockedDefaultBrowser() { return mocked_is_default_; }

  void SetMockedDefaultBrowserStatus(bool is_default) {
    mocked_is_default_ = is_default;
#if BUILDFLAG(IS_ANDROID)
    monitor_->OnDefaultBrowserStateReceived(is_default);
#endif
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<DefaultBrowserMonitor> monitor_;
  bool mocked_is_default_ = false;
};

TEST_F(DefaultBrowserMonitorUnitTest, ReportsIsNotDefaultState) {
  SetMockedDefaultBrowserStatus(false);
  CreateMonitor();

  histogram_tester_.ExpectUniqueSample(kDefaultBrowserHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kDefaultBrowserDailyHistogramName,
                                       INT_MAX - 1, 1);
}

TEST_F(DefaultBrowserMonitorUnitTest, ReportsIsDefaultState) {
  SetMockedDefaultBrowserStatus(true);
  CreateMonitor();

  histogram_tester_.ExpectUniqueSample(kDefaultBrowserHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kDefaultBrowserDailyHistogramName, 1, 1);
}

TEST_F(DefaultBrowserMonitorUnitTest, SwitchingBetweenNoAndYesStates) {
  // Start with "no" state (false)
  SetMockedDefaultBrowserStatus(false);
  CreateMonitor();

  histogram_tester_.ExpectUniqueSample(kDefaultBrowserHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kDefaultBrowserDailyHistogramName,
                                       INT_MAX - 1, 1);

  // Switch to "yes" state (true) and fast forward to trigger next check
  SetMockedDefaultBrowserStatus(true);
#if !BUILDFLAG(IS_ANDROID)
  task_environment_.FastForwardBy(base::Hours(4));
#endif

  histogram_tester_.ExpectBucketCount(kDefaultBrowserHistogramName, 1, 1);
  histogram_tester_.ExpectBucketCount(kDefaultBrowserDailyHistogramName, 1, 1);

  // Switch back to "no" state (false) and fast forward again
  SetMockedDefaultBrowserStatus(false);
#if !BUILDFLAG(IS_ANDROID)
  task_environment_.FastForwardBy(base::Hours(4));
#endif

  histogram_tester_.ExpectBucketCount(kDefaultBrowserHistogramName, 0, 2);
  histogram_tester_.ExpectBucketCount(kDefaultBrowserDailyHistogramName,
                                      INT_MAX - 1, 2);

  histogram_tester_.ExpectTotalCount(kDefaultBrowserHistogramName, 3);
  histogram_tester_.ExpectTotalCount(kDefaultBrowserDailyHistogramName, 3);
}

}  // namespace misc_metrics
