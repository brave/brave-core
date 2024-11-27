/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/test/launcher/teamcity_reporter.h"

#include <iostream>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/test/launcher/test_result.h"

namespace base {
namespace {

// This switch enables the TeamcityReporter even when the TEAMCITY_VERSION
// environment variable is not set.
constexpr char kTestLauncherEnableTeamcityReporter[] =
    "test-launcher-enable-teamcity-reporter";

// This switch disables the TeamcityReporter even when the TEAMCITY_VERSION
// environment variable is set.
constexpr char kTestLauncherDisableTeamcityReporter[] =
    "test-launcher-disable-teamcity-reporter";

// This switch enables the TeamcityReporter to ignore preliminary test failures
// when test retries are enabled, reporting only the final result of each test.
constexpr char kTestLauncherTeamcityReporterIgnorePreliminaryFailures[] =
    "test-launcher-teamcity-reporter-ignore-preliminary-failures";

// Returns the name of the current executable, excluding the extension.
std::string GetExecutableName() {
  return PathService::CheckedGet(FILE_EXE)
      .BaseName()
      .RemoveFinalExtension()
      .AsUTF8Unsafe();
}

}  // namespace

constexpr std::string_view TeamcityReporter::kPreliminaryFailureIgnoreMessage =
    "Failure ignored, expecting a retry";

constexpr std::string_view TeamcityReporter::kTestSkippedIgnoreMessage =
    "Skipped, possibly because of a previous failure";

constexpr std::string_view TeamcityReporter::kNotRetriedMessage =
    "NOT_RETRIED (suite early exit)";

TeamcityReporter::TestFailure::TestFailure() = default;
TeamcityReporter::TestFailure::~TestFailure() = default;

// static
std::unique_ptr<TeamcityReporter> TeamcityReporter::MaybeCreate() {
  const auto environment = Environment::Create();
  const CommandLine* command_line = CommandLine::ForCurrentProcess();

  const bool should_enable =
      environment->HasVar("TEAMCITY_VERSION") ||
      command_line->HasSwitch(kTestLauncherEnableTeamcityReporter);

  const bool should_disable =
      command_line->HasSwitch(kTestLauncherDisableTeamcityReporter);

  if (should_enable && !should_disable) {
    const bool ignore_preliminary_failures = command_line->HasSwitch(
        kTestLauncherTeamcityReporterIgnorePreliminaryFailures);
    return std::make_unique<TeamcityReporter>(std::cout, GetExecutableName(),
                                              ignore_preliminary_failures);
  }

  return nullptr;
}

TeamcityReporter::TeamcityReporter(std::ostream& ostream,
                                   std::string suite_name,
                                   bool ignore_preliminary_failures)
    : tsm_(ostream),
      suite_name_(std::move(suite_name)),
      ignore_preliminary_failures_(ignore_preliminary_failures) {
  LogSuiteStarted();
}

TeamcityReporter::~TeamcityReporter() {
  LogSuiteFinished();
}

void TeamcityReporter::SetRetryLimit(size_t retry_limit) {
  CHECK_EQ(test_suite_stage_, TestSuiteStage::kSuiteStarted);
  retry_limit_ = retry_limit;
  tsm_.TestRetrySupport(retry_limit_ != 0);
}

void TeamcityReporter::OnTestStarted(const TestResult& result) {
  CHECK(test_suite_stage_ == TestSuiteStage::kSuiteStarted ||
        test_suite_stage_ == TestSuiteStage::kTestFinished)
      << static_cast<int>(test_suite_stage_);
  tsm_.TestStarted(result.full_name);
  test_suite_stage_ = TestSuiteStage::kTestStarted;
}

void TeamcityReporter::OnTestResult(const TestResult& result) {
  CHECK_EQ(test_suite_stage_, TestSuiteStage::kTestStarted);
  switch (result.status) {
    case TestResult::TEST_SUCCESS:
      ClearIgnoredTestFailure(result);
      break;
    case TestResult::TEST_FAILURE:
    case TestResult::TEST_FAILURE_ON_EXIT:
    case TestResult::TEST_TIMEOUT:
    case TestResult::TEST_CRASH:
    case TestResult::TEST_EXCESSIVE_OUTPUT:
    case TestResult::TEST_UNKNOWN:
    case TestResult::TEST_NOT_RUN:
      if (ShouldIgnoreTestFailure(result)) {
        tsm_.TestIgnored(result.full_name, kPreliminaryFailureIgnoreMessage);
      } else {
        tsm_.TestFailed(result.full_name, result.StatusAsString());
      }
      break;
    case TestResult::TEST_SKIPPED:
      NOTREACHED() << "TEST_SKIPPED is unexpected. Please check "
                      "AddTestResult() override.";
  }
  test_suite_stage_ = TestSuiteStage::kTestHasResult;
}

void TeamcityReporter::OnTestFinished(const TestResult& result) {
  CHECK(test_suite_stage_ == TestSuiteStage::kTestHasResult ||
        (test_suite_stage_ == TestSuiteStage::kTestStarted &&
         result.status == TestResult::TEST_SKIPPED))
      << static_cast<int>(test_suite_stage_);
  if (result.status == TestResult::TEST_SKIPPED) {
    // This is not a failure nor a success. Mark the test as ignored to not add
    // it into "successful/failed" lists.
    tsm_.TestIgnored(result.full_name, kTestSkippedIgnoreMessage);
  }
  tsm_.TestFinished(result.full_name, result.elapsed_time);
  test_suite_stage_ = TestSuiteStage::kTestFinished;
}

void TeamcityReporter::OnBrokenTestEarlyExit() {
  LogSuiteFinished();
}

void TeamcityReporter::LogSuiteStarted() {
  CHECK_EQ(test_suite_stage_, TestSuiteStage::kNone);
  tsm_.TestSuiteStarted(suite_name_);
  test_suite_stage_ = TestSuiteStage::kSuiteStarted;
}

void TeamcityReporter::LogSuiteFinished() {
  if (test_suite_stage_ != TestSuiteStage::kSuiteFinished) {
    ReportIgnoredTestFailures();
    tsm_.TestSuiteFinished(suite_name_);
    test_suite_stage_ = TestSuiteStage::kSuiteFinished;
  }
}

bool TeamcityReporter::ShouldIgnoreTestFailure(const TestResult& result) {
  CHECK_EQ(test_suite_stage_, TestSuiteStage::kTestStarted);
  CHECK_NE(result.status, TestResult::TEST_SUCCESS);

  if (ignore_preliminary_failures_ && retry_limit_ > 0) {
    auto& test_failure = ignored_test_failures_[result.full_name];
    if (test_failure.attempt < retry_limit_) {
      // The test has failed, but we're ignoring it for now.
      ++test_failure.attempt;
      // Store the result to report it on early exit.
      test_failure.result = result;
      return true;
    } else {
      // The test is about to be reported. Unset the result to avoid double
      // reporting on early exit.
      test_failure.result.reset();
    }
  }

  return false;
}

void TeamcityReporter::ClearIgnoredTestFailure(const TestResult& result) {
  CHECK_EQ(test_suite_stage_, TestSuiteStage::kTestStarted);
  CHECK_EQ(result.status, TestResult::TEST_SUCCESS);
  ignored_test_failures_.erase(result.full_name);
}

void TeamcityReporter::ReportIgnoredTestFailures() {
  for (const auto& [test_name, test_failure] : ignored_test_failures_) {
    if (test_failure.result) {
      const TestResult& result = *test_failure.result;
      tsm_.TestStarted(test_name);
      tsm_.TestFailed(
          test_name,
          JoinString({kNotRetriedMessage, result.StatusAsString()}, "\n"),
          result.output_snippet);
      tsm_.TestFinished(test_name, result.elapsed_time);
    }
  }
}

}  // namespace base
