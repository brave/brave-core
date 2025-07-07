/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_extra_parts_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/run_until.h"
#include "brave/browser/updater/updater_p3a.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

// This test checks that BraveBrowserMainExtraPartsP3A::PostBrowserStart
// schedules a task to report update metrics. The logic behind which exact
// metrics are reported is tested in updater_p3a_unittest.cc.
class BraveBrowserMainExtraPartsP3ATest : public testing::Test {
 public:
  BraveBrowserMainExtraPartsP3ATest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~BraveBrowserMainExtraPartsP3ATest() override {}

 protected:
  void SetUp() override {
    brave_updater::SetLastLaunchVersionForTesting("0.0.0.0",
                                                  local_state_.Get());
  }

  void RunPostBrowserStart() {
    BraveBrowserMainExtraPartsP3A test_target;
    test_target.PostBrowserStart();
    ASSERT_TRUE(base::test::RunUntil(
        [&]() { return test_target.WasPostBrowserStartCalled(); }));
  }

  void ExpectHistogramCount(int expected_count) {
    histogram_tester_.ExpectTotalCount(
        brave_updater::kUpdateStatusHistogramName, expected_count);
  }

  base::HistogramTester histogram_tester_;

 private:
  content::BrowserTaskEnvironment task_environment_;
  ScopedTestingLocalState local_state_;
};

TEST_F(BraveBrowserMainExtraPartsP3ATest, PostBrowserStartSchedulesP3ATask) {
  RunPostBrowserStart();
  ExpectHistogramCount(1);
}

TEST_F(BraveBrowserMainExtraPartsP3ATest, PostBrowserStartHandlesShutdown) {
  TestingBrowserProcess::GetGlobal()->SetShuttingDown(true);
  RunPostBrowserStart();
  ExpectHistogramCount(0);
  // Reset the state for the next test.
  TestingBrowserProcess::GetGlobal()->SetShuttingDown(false);
}
