/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_REPORTER_H_
#define BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_REPORTER_H_

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "base/test/launcher/test_result.h"
#include "brave/base/test/launcher/teamcity_service_messages.h"

namespace base {

// Reports test results to Teamcity using Service Messages.
class TeamcityReporter {
 public:
  // Creates the reporter if TEAMCITY_VERSION environment variable is set or if
  // a command line flag is passed.
  static std::unique_ptr<TeamcityReporter> MaybeCreate();

  TeamcityReporter(std::ostream& ostream,
                   std::string suite_name,
                   bool ignore_preliminary_failures);
  TeamcityReporter(const TeamcityReporter&) = delete;
  TeamcityReporter& operator=(const TeamcityReporter&) = delete;
  ~TeamcityReporter();

  // Enable or disable retry support on Teamcity. With this option enabled, the
  // successful run of a test will mute its previous failure.
  void SetRetryLimit(size_t retry_limit);

  void OnTestStarted(const TestResult& result);
  void OnTestResult(const TestResult& result);
  void OnTestFinished(const TestResult& result);

  // This is called when a lot of tests have failed and TestLauncher decides to
  // do an early exit.
  void OnBrokenTestEarlyExit();

  // Public for tests.
  static const std::string_view kPreliminaryFailureIgnoreMessage;
  static const std::string_view kTestSkippedIgnoreMessage;
  static const std::string_view kNotRetriedMessage;

 private:
  enum class TestSuiteStage {
    kNone,
    kSuiteStarted,
    kTestStarted,
    kTestHasResult,
    kTestFinished,
    kSuiteFinished,
  };

  struct TestFailure {
    TestFailure();
    TestFailure(const TestFailure&) = delete;
    TestFailure& operator=(const TestFailure&) = delete;
    ~TestFailure();

    size_t attempt = 0;
    std::optional<TestResult> result;
  };

  void LogSuiteStarted();
  void LogSuiteFinished();

  // A test failure can be ignored if it is a preliminary failure which may be
  // fixed on retry.
  bool ShouldIgnoreTestFailure(const TestResult& result);
  // If a test is successful on retry, the previous failure should be cleared to
  // not report on shutdown.
  void ClearIgnoredTestFailure(const TestResult& result);
  // Report ignored test failures. Used on shutdown.
  void ReportIgnoredTestFailures();

  TeamcityServiceMessages tsm_;
  const std::string suite_name_;

  // Skips initial failures when retries are on, reporting only final test
  // results. Useful for test suites with flaky tests, where flakiness reporting
  // is not a concern and no fix is intended (e.g. upstream tests).
  const bool ignore_preliminary_failures_;

  // The number of retries allowed for each test.
  size_t retry_limit_ = 0;

  // The current test suite stage. This is used to ensure that the test
  // callbacks are called in the correct order.
  TestSuiteStage test_suite_stage_ = TestSuiteStage::kNone;

  // Test failures to be reported on early exit if ignore_preliminary_failures_
  // is enabled.
  std::map<std::string, TestFailure> ignored_test_failures_;
};

}  // namespace base

#endif  // BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_REPORTER_H_
