/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_TEST_LAUNCHER_TEST_LAUNCHER_H_
#define BRAVE_CHROMIUM_SRC_BASE_TEST_LAUNCHER_TEST_LAUNCHER_H_

#include <memory>

#include "base/test/launcher/test_result.h"

namespace base {
class TestLauncher;
using TestLauncher_BraveImpl = TestLauncher;
}  // namespace base

#define TestLauncher TestLauncher_ChromiumImpl
#define OnTestFinished                                                  \
  NotUsed();                                                            \
  friend TestLauncher_BraveImpl;                                        \
  virtual const TestResult& OnTestResult(const TestResult& result) = 0; \
  virtual void OnTestFinished
#define MaybeSaveSummaryAsJSON virtual MaybeSaveSummaryAsJSON

#include "src/base/test/launcher/test_launcher.h"  // IWYU pragma: export

#undef TestLauncher
#undef OnTestFinished
#undef MaybeSaveSummaryAsJSON

namespace base {

class TeamcityReporter;

class TestLauncher : public TestLauncher_ChromiumImpl {
 public:
  TestLauncher(TestLauncherDelegate* launcher_delegate,
               size_t parallel_jobs,
               size_t retry_limit = 1U);
  ~TestLauncher() override;

  void OnTestFinished(const TestResult& result) override;

 private:
  void CreateAndStartThreadPool(size_t num_parallel_jobs) override;
  const TestResult& OnTestResult(const TestResult& result) override;
  void MaybeSaveSummaryAsJSON(
      const std::vector<std::string>& additional_tags) override;

  std::unique_ptr<TeamcityReporter> teamcity_reporter_;
};

}  // namespace base

#endif  // BRAVE_CHROMIUM_SRC_BASE_TEST_LAUNCHER_TEST_LAUNCHER_H_
