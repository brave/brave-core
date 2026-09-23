/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/test/launcher/teamcity_reporter.h"

#include <sstream>

#include "base/test/launcher/test_result.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

class TeamcityReporterTest : public testing::TestWithParam<bool> {
 protected:
  void SetUp() override {
    // Instantiate reporter with enabled/disable preliminary failures reporting.
    // The reporter should behave the same, because the retry limit is not set.
    teamcity_reporter_ = std::make_unique<TeamcityReporter>(
        mock_ostream_, "my_suite", GetParam());
    EXPECT_EQ(GetStr(), "##teamcity[testSuiteStarted name='my_suite']\n");
  }

  void TearDown() override {
    if (teamcity_reporter_) {
      teamcity_reporter_.reset();
      EXPECT_EQ(GetStr(), "##teamcity[testSuiteFinished name='my_suite']\n");
    }
  }

  std::string GetStr() {
    std::string str = mock_ostream_.str();
    ClearStr();
    return str;
  }

  void ClearStr() { mock_ostream_.str(""); }

  std::stringstream mock_ostream_;
  std::unique_ptr<TeamcityReporter> teamcity_reporter_;
};

TEST_P(TeamcityReporterTest, SetRetryLimit) {
  teamcity_reporter_->SetRetryLimit(1);
  EXPECT_EQ(GetStr(), "##teamcity[testRetrySupport enabled='true']\n");

  teamcity_reporter_->SetRetryLimit(0);
  EXPECT_EQ(GetStr(), "##teamcity[testRetrySupport enabled='false']\n");
}

TEST_P(TeamcityReporterTest, TestSuccessful) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_SUCCESS;
  result.elapsed_time = Milliseconds(100);
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testFinished name='TestSuite.TestName' duration='100']\n");
}

TEST_P(TeamcityReporterTest, TestFailed) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_FAILURE;
  result.elapsed_time = Milliseconds(100);
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testFailed name='TestSuite.TestName' message='FAILURE']\n"
      "##teamcity[testFinished name='TestSuite.TestName' duration='100']\n");
}

TEST_P(TeamcityReporterTest, TestSkipped) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_SKIPPED;
  result.elapsed_time = Milliseconds(100);
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(GetStr(),
            "##teamcity[testStarted name='TestSuite.TestName' "
            "captureStandardOutput='true']\n"
            "##teamcity[testIgnored name='TestSuite.TestName' "
            "message='" +
                std::string(TeamcityReporter::kTestSkippedIgnoreMessage) +
                "']\n"
                "##teamcity[testFinished name='TestSuite.TestName' "
                "duration='100']\n");
}

TEST_P(TeamcityReporterTest, OnBrokenTestEarlyExit) {
  teamcity_reporter_->OnBrokenTestEarlyExit();
  EXPECT_EQ(GetStr(), "##teamcity[testSuiteFinished name='my_suite']\n");

  teamcity_reporter_->OnBrokenTestEarlyExit();
  EXPECT_EQ(GetStr(), "");

  teamcity_reporter_.reset();
  EXPECT_EQ(GetStr(), "");
}

TEST_P(TeamcityReporterTest, MissingResultOnSuccess) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_SUCCESS;
  teamcity_reporter_->OnTestStarted(result);
  ClearStr();
  EXPECT_DEATH_IF_SUPPORTED(teamcity_reporter_->OnTestFinished(result), "");
}

TEST_P(TeamcityReporterTest, MissingResultOnFailure) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_FAILURE;
  teamcity_reporter_->OnTestStarted(result);
  ClearStr();
  EXPECT_DEATH_IF_SUPPORTED(teamcity_reporter_->OnTestFinished(result), "");
}

TEST_P(TeamcityReporterTest, UnexpectedResultOnSkipped) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_SKIPPED;
  teamcity_reporter_->OnTestStarted(result);
  ClearStr();
  EXPECT_DEATH_IF_SUPPORTED(teamcity_reporter_->OnTestResult(result), "");
}

TEST_P(TeamcityReporterTest, MissingStart) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_SUCCESS;
  EXPECT_DEATH_IF_SUPPORTED(teamcity_reporter_->OnTestResult(result), "");
  EXPECT_DEATH_IF_SUPPORTED(teamcity_reporter_->OnTestFinished(result), "");
}

TEST_P(TeamcityReporterTest, NoReportingAfterEarlyExit) {
  teamcity_reporter_->OnBrokenTestEarlyExit();
  ClearStr();

  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_SUCCESS;
  EXPECT_DEATH_IF_SUPPORTED(teamcity_reporter_->OnTestStarted(result), "");

  teamcity_reporter_.reset();
}

INSTANTIATE_TEST_SUITE_P(, TeamcityReporterTest, testing::Bool());

class TeamcityReporterIgnorePreliminaryFailuresTest
    : public TeamcityReporterTest {
 protected:
  void SetUp() override {
    teamcity_reporter_ =
        std::make_unique<TeamcityReporter>(mock_ostream_, "my_suite", true);
    teamcity_reporter_->SetRetryLimit(1);
    EXPECT_EQ(GetStr(),
              "##teamcity[testSuiteStarted name='my_suite']\n"
              "##teamcity[testRetrySupport enabled='true']\n");
  }
};

TEST_F(TeamcityReporterIgnorePreliminaryFailuresTest, TestSuccessful) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_SUCCESS;
  result.elapsed_time = Milliseconds(100);
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testFinished name='TestSuite.TestName' duration='100']\n");
}

TEST_F(TeamcityReporterIgnorePreliminaryFailuresTest, TestFailedOnRetry) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_FAILURE;
  result.elapsed_time = Milliseconds(100);
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testIgnored name='TestSuite.TestName' "
      "message='" +
          std::string(TeamcityReporter::kPreliminaryFailureIgnoreMessage) +
          "']\n"
          "##teamcity[testFinished name='TestSuite.TestName' "
          "duration='100']\n");

  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testFailed name='TestSuite.TestName' message='FAILURE']\n"
      "##teamcity[testFinished name='TestSuite.TestName' duration='100']\n");
}

TEST_F(TeamcityReporterIgnorePreliminaryFailuresTest, TestSuccessfulOnRetry) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_FAILURE;
  result.elapsed_time = Milliseconds(100);
  result.output_snippet = "output";
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testIgnored name='TestSuite.TestName' "
      "message='" +
          std::string(TeamcityReporter::kPreliminaryFailureIgnoreMessage) +
          "']\n"
          "##teamcity[testFinished name='TestSuite.TestName' "
          "duration='100']\n");

  result.status = TestResult::TEST_SUCCESS;
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testFinished name='TestSuite.TestName' duration='100']\n");
}

TEST_F(TeamcityReporterIgnorePreliminaryFailuresTest, OnBrokenTestEarlyExit) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_FAILURE;
  result.elapsed_time = Milliseconds(100);
  result.output_snippet = "output";
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testIgnored name='TestSuite.TestName' "
      "message='" +
          std::string(TeamcityReporter::kPreliminaryFailureIgnoreMessage) +
          "']\n"
          "##teamcity[testFinished name='TestSuite.TestName' "
          "duration='100']\n");

  teamcity_reporter_->OnBrokenTestEarlyExit();
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testFailed name='TestSuite.TestName' "
      "message='" +
          std::string(TeamcityReporter::kNotRetriedMessage) +
          "|nFAILURE' details='output']\n"
          "##teamcity[testFinished name='TestSuite.TestName' duration='100']\n"
          "##teamcity[testSuiteFinished name='my_suite']\n");

  teamcity_reporter_.reset();
  EXPECT_EQ(GetStr(), "");
}

TEST_F(TeamcityReporterIgnorePreliminaryFailuresTest, Shutdown) {
  TestResult result;
  result.full_name = "TestSuite.TestName";
  result.status = TestResult::TEST_FAILURE;
  result.elapsed_time = Milliseconds(100);
  result.output_snippet = "output";
  teamcity_reporter_->OnTestStarted(result);
  teamcity_reporter_->OnTestResult(result);
  teamcity_reporter_->OnTestFinished(result);
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testIgnored name='TestSuite.TestName' "
      "message='" +
          std::string(TeamcityReporter::kPreliminaryFailureIgnoreMessage) +
          "']\n"
          "##teamcity[testFinished name='TestSuite.TestName' "
          "duration='100']\n");

  teamcity_reporter_.reset();
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='TestSuite.TestName' "
      "captureStandardOutput='true']\n"
      "##teamcity[testFailed name='TestSuite.TestName' "
      "message='" +
          std::string(TeamcityReporter::kNotRetriedMessage) +
          "|nFAILURE' details='output']\n"
          "##teamcity[testFinished name='TestSuite.TestName' duration='100']\n"
          "##teamcity[testSuiteFinished name='my_suite']\n");
}

}  // namespace base
