/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_REPORTER_H_
#define BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_REPORTER_H_

#include <memory>
#include <ostream>
#include <string>

#include "brave/base/test/launcher/teamcity_service_messages.h"

namespace base {

struct TestResult;

// Reports test results to Teamcity using Service Messages.
class TeamcityReporter {
 public:
  // Creates the reporter if TEAMCITY_VERSION environment variable is set or if
  // a command line flag is passed.
  static std::unique_ptr<TeamcityReporter> MaybeCreate();

  TeamcityReporter(std::ostream& ostream, std::string suite_name);
  TeamcityReporter(const TeamcityReporter&) = delete;
  TeamcityReporter& operator=(const TeamcityReporter&) = delete;
  ~TeamcityReporter();

  // Enable or disable retry support on Teamcity. With this option enabled, the
  // successful run of a test will mute its previous failure.
  void EnableRetrySupport(bool enabled);

  void OnTestStarted(const TestResult& result);
  void OnTestResult(const TestResult& result);
  void OnTestFinished(const TestResult& result);

  // This is called when a lot of tests have failed and TestLauncher decides to
  // do an early exit.
  void OnBrokenTestEarlyExit();

 private:
  enum class TestSuiteStage {
    kNone,
    kSuiteStarted,
    kTestStarted,
    kTestHasResult,
    kTestFinished,
    kSuiteFinished,
  };

  void LogSuiteStarted();
  void LogSuiteFinished();

  TeamcityServiceMessages tsm_;
  const std::string suite_name_;

  // The current test suite stage. This is used to ensure that the test
  // callbacks are called in the correct order.
  TestSuiteStage test_suite_stage_ = TestSuiteStage::kNone;
};

}  // namespace base

#endif  // BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_REPORTER_H_
