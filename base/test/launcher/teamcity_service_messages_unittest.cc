/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/test/launcher/teamcity_service_messages.h"

#include <memory>
#include <string>

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

class TeamcityServiceMessagesTest : public testing::Test {
 protected:
  void SetUp() override {
    tsm_ = std::make_unique<TeamcityServiceMessages>(mock_ostream_);
  }

  void TearDown() override { tsm_.reset(); }

  std::string GetStr() {
    std::string str = mock_ostream_.str();
    ClearStr();
    return str;
  }

  void ClearStr() { mock_ostream_.str(""); }

  void ExpectPropertyEscaped(std::string_view property_value,
                             std::string_view escaped_property_value) {
    {
      TeamcityServiceMessages::Message message(mock_ostream_, "dummy");
      message.WriteProperty("property_name", property_value);
    }
    if (!escaped_property_value.empty()) {
      std::string expected = "##teamcity[dummy property_name='" +
                             std::string(escaped_property_value) + "']\n";
      EXPECT_EQ(GetStr(), expected);
    } else {
      EXPECT_EQ(GetStr(), "##teamcity[dummy]\n");
    }
  }

  std::stringstream mock_ostream_;
  std::unique_ptr<TeamcityServiceMessages> tsm_;
};

TEST_F(TeamcityServiceMessagesTest, WritePropertyWithSpecialCharacters) {
  struct TestCase {
    std::string_view property_value;
    std::string_view escaped_property_value;
  } test_cases[] = {
      {"property_value\n\r'|[]", "property_value|n|r|'|||[|]"},
      {"\na\rbcde'test| [ ] ", "|na|rbcde|'test|| |[ |] "},
      {"a'b\n]| '\rc", "a|'b|n|]|| |'|rc"},
      {"property_value", "property_value"},
      {"", ""},
  };
  for (const auto& test_case : test_cases) {
    ExpectPropertyEscaped(test_case.property_value,
                          test_case.escaped_property_value);
  }
}

TEST_F(TeamcityServiceMessagesTest, TestRetrySupport) {
  tsm_->TestRetrySupport(true);
  EXPECT_EQ(GetStr(), "##teamcity[testRetrySupport enabled='true']\n");

  tsm_->TestRetrySupport(false);
  EXPECT_EQ(GetStr(), "##teamcity[testRetrySupport enabled='false']\n");
}

TEST_F(TeamcityServiceMessagesTest, TestSuiteStarted) {
  tsm_->TestSuiteStarted("TestSuite1");
  EXPECT_EQ(GetStr(), "##teamcity[testSuiteStarted name='TestSuite1']\n");
}

TEST_F(TeamcityServiceMessagesTest, TestSuiteFinished) {
  tsm_->TestSuiteFinished("TestSuite1");
  EXPECT_EQ(GetStr(), "##teamcity[testSuiteFinished name='TestSuite1']\n");
}

TEST_F(TeamcityServiceMessagesTest, TestStarted) {
  tsm_->TestStarted("Test1");
  EXPECT_EQ(
      GetStr(),
      "##teamcity[testStarted name='Test1' captureStandardOutput='true']\n");
}

TEST_F(TeamcityServiceMessagesTest, TestFailed) {
  tsm_->TestFailed("Test1", "Test failed", "Details");
  EXPECT_EQ(GetStr(),
            "##teamcity[testFailed name='Test1' message='Test failed' "
            "details='Details']\n");
}

TEST_F(TeamcityServiceMessagesTest, TestIgnored) {
  tsm_->TestIgnored("Test1", "Test ignored");
  EXPECT_EQ(GetStr(),
            "##teamcity[testIgnored name='Test1' message='Test ignored']\n");
}

TEST_F(TeamcityServiceMessagesTest, TestFinished) {
  tsm_->TestFinished("Test1", Milliseconds(100));
  EXPECT_EQ(GetStr(), "##teamcity[testFinished name='Test1' duration='100']\n");
}

}  // namespace base
