// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/history_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/click_tool_request.h"
#include "chrome/browser/actor/tools/history_tool_request.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class HistoryToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<HistoryTool>(mock_task_provider_.get());
  }

 protected:
  std::string CreateToolInputJson(const std::string& direction) {
    base::Value::Dict dict;
    dict.Set("direction", direction);

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  void VerifySuccess(const std::string& input_json,
                     const std::string& expected_direction) {
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "History");
    auto* history_request =
        static_cast<actor::HistoryToolRequest*>(tool_request.get());

    if (expected_direction == "back") {
      EXPECT_TRUE(action.has_back());
      const auto& back_action = action.back();
      EXPECT_EQ(back_action.tab_id(), test_tab_handle_.raw_value());
      EXPECT_EQ(history_request->direction_,
                actor::HistoryToolRequest::Direction::kBack);
    } else {
      EXPECT_TRUE(action.has_forward());
      const auto& forward_action = action.forward();
      EXPECT_EQ(forward_action.tab_id(), test_tab_handle_.raw_value());
      EXPECT_EQ(history_request->direction_,
                actor::HistoryToolRequest::Direction::kForward);
    }
  }
};

TEST_F(HistoryToolTest, ValidInputBack) {
  std::string input_json = CreateToolInputJson("back");

  VerifySuccess(input_json, "back");
}

TEST_F(HistoryToolTest, ValidInputForward) {
  std::string input_json = CreateToolInputJson("forward");

  VerifySuccess(input_json, "forward");
}

TEST_F(HistoryToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(HistoryToolTest, MissingDirection) {
  std::string input_json = R"({})";

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(HistoryToolTest, InvalidDirection) {
  std::string input_json = CreateToolInputJson("invalid_direction");

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(HistoryToolTest, InvalidDirectionType) {
  std::string input_json = R"({
    "direction": 123
  })";

  RunWithExpectedError(FROM_HERE, input_json);
}

}  // namespace ai_chat
