/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_TEST_LAUNCHER_TEST_LAUNCHER_H_
#define BRAVE_CHROMIUM_SRC_BASE_TEST_LAUNCHER_TEST_LAUNCHER_H_

#include "base/test/launcher/test_result.h"

namespace base {
class TestLauncher;
using TestLauncher_BraveImpl = TestLauncher;
}  // namespace base

#define TestLauncher TestLauncher_ChromiumImpl
#define OnTestFinished                                   \
  NotUsed();                                             \
  friend TestLauncher_BraveImpl;                         \
  virtual void OnTestResult(const TestResult& result) {} \
  virtual void OnTestFinished
#define MaybeSaveSummaryAsJSON virtual MaybeSaveSummaryAsJSON

#include "src/base/test/launcher/test_launcher.h"  // IWYU pragma: export

#undef OnTestFinished
#undef MaybeSaveSummaryAsJSON
#undef TestLauncher

namespace base {

class TestLauncher : public TestLauncher_ChromiumImpl {
 public:
  TestLauncher(TestLauncherDelegate* launcher_delegate,
               size_t parallel_jobs,
               size_t retry_limit = 1U);
  ~TestLauncher() override;

  void OnTestFinished(const TestResult& result) override;

 private:
  void LogTeamcityTestStart(const TestResult& result) const;
  void LogTeamcityTestFinish(const TestResult& result) const;

  void OnTestResult(const TestResult& result) override;
  void MaybeSaveSummaryAsJSON(
      const std::vector<std::string>& additional_tags) override;

  bool teamcity_retry_support_set_ = false;
  raw_ptr<const TestResult> current_test_result_ = nullptr;
};

}  // namespace base

#endif  // BRAVE_CHROMIUM_SRC_BASE_TEST_LAUNCHER_TEST_LAUNCHER_H_
