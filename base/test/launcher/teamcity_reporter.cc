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
#include "base/path_service.h"
#include "base/test/launcher/test_result.h"

namespace base {
namespace {

// This switch enables the TeamcityReporter even when the TEAMCITY_VERSION
// environment variable is not set.
constexpr char kTestLauncherTeamcityReporter[] =
    "test-launcher-teamcity-reporter";

// This switch disables the TeamcityReporter even when the TEAMCITY_VERSION
// environment variable is set.
constexpr char kTestLauncherNoTeamcityReporter[] =
    "test-launcher-no-teamcity-reporter";

// Returns the name of the current executable, excluding the extension.
std::string GetExecutableName() {
  base::FilePath file_exe = base::PathService::CheckedGet(base::FILE_EXE);
  file_exe = file_exe.BaseName().RemoveFinalExtension();
  return file_exe.AsUTF8Unsafe();
}

}  // namespace

// static
std::unique_ptr<TeamcityReporter> TeamcityReporter::MaybeCreate() {
  const auto environment = Environment::Create();
  const CommandLine* command_line = CommandLine::ForCurrentProcess();

  const bool should_enable =
      environment->HasVar("TEAMCITY_VERSION") ||
      command_line->HasSwitch(kTestLauncherTeamcityReporter);

  const bool should_disable =
      command_line->HasSwitch(kTestLauncherNoTeamcityReporter);

  if (should_enable && !should_disable) {
    return std::make_unique<TeamcityReporter>(std::cout, GetExecutableName());
  }

  return nullptr;
}

TeamcityReporter::TeamcityReporter(std::ostream& ostream,
                                   std::string suite_name)
    : tsm_(ostream), suite_name_(std::move(suite_name)) {
  LogSuiteStarted();
}

TeamcityReporter::~TeamcityReporter() {
  LogSuiteFinished();
}

void TeamcityReporter::EnableRetrySupport(bool enabled) {
  CHECK_EQ(test_suite_stage_, TestSuiteStage::kSuiteStarted);
  tsm_.TestRetrySupport(enabled);
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
      break;
    case TestResult::TEST_FAILURE:
    case TestResult::TEST_FAILURE_ON_EXIT:
    case TestResult::TEST_TIMEOUT:
    case TestResult::TEST_CRASH:
    case TestResult::TEST_EXCESSIVE_OUTPUT:
    case TestResult::TEST_UNKNOWN:
    case TestResult::TEST_NOT_RUN:
      tsm_.TestFailed(result.full_name);
      break;
    case TestResult::TEST_SKIPPED:
      CHECK(false) << "TEST_SKIPPED is unexpected. Please check "
                      "AddTestResult() override.";
      break;
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
    tsm_.TestIgnored(result.full_name);
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
    tsm_.TestSuiteFinished(suite_name_);
    test_suite_stage_ = TestSuiteStage::kSuiteFinished;
  }
}

}  // namespace base
