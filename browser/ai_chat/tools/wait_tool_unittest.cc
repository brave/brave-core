// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/wait_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "chrome/browser/actor/tools/wait_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class WaitToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<WaitTool>(mock_task_provider_.get());
  }

  // Creates a valid wait JSON with the given wait time
  std::string CreateToolInputJson(int wait_time_ms) {
    base::Value::Dict dict;
    dict.Set("wait_time_ms", wait_time_ms);

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  // Verify wait action properties and conversions
  void VerifySuccess(const std::string& input_json, int expected_wait_time_ms) {
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, false);

    EXPECT_TRUE(action.has_wait());

    const auto& wait_action = action.wait();
    EXPECT_EQ(wait_action.wait_time_ms(), expected_wait_time_ms);

    auto* wait_request =
        static_cast<actor::WaitToolRequest*>(tool_request.get());
    EXPECT_NE(wait_request, nullptr);
    // Note: WaitToolRequest doesn't store wait_time_ms as a member, it's
    // converted to a TimeDelta in the constructor, so we can't easily verify
    // the exact value
  }
};

TEST_F(WaitToolTest, ValidInputShortWait) {
  std::string input_json = CreateToolInputJson(1000);  // 1 second

  // Verify wait action properties
  VerifySuccess(input_json, 1000);
}

TEST_F(WaitToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(WaitToolTest, MissingWaitTime) {
  RunWithExpectedError(FROM_HERE, R"({})");
}

TEST_F(WaitToolTest, NegativeWaitTime) {
  RunWithExpectedError(FROM_HERE, CreateToolInputJson(-1000));
}

TEST_F(WaitToolTest, ZeroWaitTime) {
  RunWithExpectedError(FROM_HERE, CreateToolInputJson(0));
}

TEST_F(WaitToolTest, InvalidWaitTimeType) {
  std::string input_json = R"({
    "wait_time_ms": "not_a_number"
  })";

  RunWithExpectedError(FROM_HERE, input_json);
}

}  // namespace ai_chat
