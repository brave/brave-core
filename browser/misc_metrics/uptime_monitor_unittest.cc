/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/misc_metrics/uptime_monitor.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class UptimeMonitorUnitTest : public testing::Test {
 public:
  UptimeMonitorUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    misc_metrics::UptimeMonitor::RegisterPrefs(local_state_.registry());

    ResetMonitor();
  }

 protected:
  void ResetMonitor() {
    usage_monitor_ = std::make_unique<UptimeMonitor>(&local_state_);
    usage_monitor_->Init();
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<UptimeMonitor> usage_monitor_;
};

#if BUILDFLAG(IS_ANDROID)
TEST_F(UptimeMonitorUnitTest, ReportUsageDuration) {
  histogram_tester_.ExpectTotalCount(kBrowserOpenTimeHistogramName, 0);

  usage_monitor_->ReportUsageDuration(base::Minutes(15));
  task_environment_.FastForwardBy(base::Hours(15));
  usage_monitor_->ReportUsageDuration(base::Minutes(4));

  histogram_tester_.ExpectTotalCount(kBrowserOpenTimeHistogramName, 0);

  task_environment_.FastForwardBy(base::Hours(9));
  usage_monitor_->ReportUsageDuration(base::Minutes(1));

  histogram_tester_.ExpectUniqueSample(kBrowserOpenTimeHistogramName, 0, 1);

  usage_monitor_->ReportUsageDuration(base::Minutes(40));
  task_environment_.FastForwardBy(base::Hours(15));
  usage_monitor_->ReportUsageDuration(base::Minutes(1));

  histogram_tester_.ExpectUniqueSample(kBrowserOpenTimeHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Hours(9));
  usage_monitor_->ReportUsageDuration(base::Minutes(1));

  histogram_tester_.ExpectBucketCount(kBrowserOpenTimeHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(kBrowserOpenTimeHistogramName, 2);

  task_environment_.FastForwardBy(base::Hours(15));
  usage_monitor_->ReportUsageDuration(base::Minutes(170));
  histogram_tester_.ExpectTotalCount(kBrowserOpenTimeHistogramName, 2);

  ResetMonitor();
  task_environment_.FastForwardBy(base::Hours(9));
  usage_monitor_->ReportUsageDuration(base::Minutes(1));

  histogram_tester_.ExpectBucketCount(kBrowserOpenTimeHistogramName, 3, 1);
  histogram_tester_.ExpectTotalCount(kBrowserOpenTimeHistogramName, 3);
}
#endif

}  // namespace misc_metrics
