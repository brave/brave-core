/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/launcher/test_launcher.h"

#include "brave/base/test/launcher/teamcity_reporter.h"

#define TestLauncher TestLauncher_ChromiumImpl
#define AddTestResult(...) AddTestResult(OnTestResult(__VA_ARGS__));

#include "src/base/test/launcher/test_launcher.cc"

#undef TestLauncher
#undef AddTestResult

namespace base {

TestLauncher::TestLauncher(TestLauncherDelegate* launcher_delegate,
                           size_t parallel_jobs,
                           size_t retry_limit)
    : TestLauncher_ChromiumImpl(launcher_delegate, parallel_jobs, retry_limit) {
  teamcity_reporter_ = TeamcityReporter::MaybeCreate();
}

TestLauncher::~TestLauncher() = default;

void TestLauncher::OnTestFinished(const TestResult& result) {
  // The order of TC log calls is important here. First we want to let TC know a
  // test is starting, then we call the original `OnTestFinished` which may
  // print the test output on failure, so it will become a part of the
  // TC-reported test. Finally we want to let TC know the test is finished so
  // any other test launcher output should not be bound to the test.
  if (teamcity_reporter_) {
    teamcity_reporter_->OnTestStarted(result);
  }

  // Upstream implementation of this method does roughly this:
  // 1. Print the test output if it has failed.
  // 2. Add test results via `results_tracker_.AddTestResult()`.
  // 3. Call exit(1) if a lot of test have failed.
  //
  // TestLauncher::OnTestResult() will be called from AddTestResult() override.
  // TestLauncher::MaybeSaveSummaryAsJSON() will be called before exit(1).
  TestLauncher_ChromiumImpl::OnTestFinished(result);

  if (teamcity_reporter_) {
    teamcity_reporter_->OnTestFinished(result);
  }
}

void TestLauncher::CreateAndStartThreadPool(size_t num_parallel_jobs) {
  // `retry_limit_` can be overridden by command line. Read its value when all
  // command line flags are parsed.
  if (teamcity_reporter_) {
    teamcity_reporter_->SetRetryLimit(retry_limit_);
  }

  TestLauncher_ChromiumImpl::CreateAndStartThreadPool(num_parallel_jobs);
}

// This is called from TestLauncher_ChromiumImpl::OnTestFinished() via
// AddTestResult() override.
const TestResult& TestLauncher::OnTestResult(const TestResult& result) {
  if (teamcity_reporter_) {
    teamcity_reporter_->OnTestResult(result);
  }
  return result;
}

void TestLauncher::MaybeSaveSummaryAsJSON(
    const std::vector<std::string>& additional_tags) {
  // This may be called from TestLauncher_ChromiumImpl::OnTestFinished() when a
  // lot of test has failed and the TestLauncher decides to do an early exit.
  if (teamcity_reporter_ &&
      Contains(additional_tags, "BROKEN_TEST_EARLY_EXIT")) {
    // TestLauncher will call exit(1) before returning from OnTestFinished(), so
    // log the test suite shutdown here while we can.
    teamcity_reporter_->OnBrokenTestEarlyExit();
  }

  TestLauncher_ChromiumImpl::MaybeSaveSummaryAsJSON(additional_tags);
}

}  // namespace base
