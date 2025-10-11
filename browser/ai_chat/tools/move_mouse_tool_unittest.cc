// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/move_mouse_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/move_mouse_tool_request.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class MoveMouseToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<MoveMouseTool>(mock_task_provider_.get());
  }

  std::string CreateToolInputJson(const base::Value::Dict& target_dict) {
    base::Value::Dict dict;
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  optimization_guide::proto::Action VerifySuccess(
      const std::string& input_json) {
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "MoveMouse");

    EXPECT_TRUE(action.has_move_mouse());
    const auto& move_mouse_action = action.move_mouse();
    EXPECT_EQ(move_mouse_action.tab_id(), test_tab_handle_.raw_value());

    // Target verification should be handled by the target_test_util methods
    EXPECT_TRUE(move_mouse_action.has_target());

    // auto* move_mouse_request =
    //     static_cast<actor::MoveMouseToolRequest*>(tool_request.get());

    // auto mojo_action = move_mouse_request->ToMojoToolAction();
    // EXPECT_TRUE(mojo_action->is_mouse_move());

    return action;
  }
};

TEST_F(MoveMouseToolTest, ValidInput) {
  // Use custom content node values
  auto target_dict =
      target_test_util::GetContentNodeTargetDict(777, "special_doc_id");
  std::string input_json = CreateToolInputJson(target_dict);

  auto action = VerifySuccess(input_json);

  // Verify target
  const auto& target = action.move_mouse().target();
  target_test_util::VerifyContentNodeTarget(target, 777, "special_doc_id");
}

TEST_F(MoveMouseToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(MoveMouseToolTest, MissingTarget) {
  RunWithExpectedError(FROM_HERE, R"({})");
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(MoveMouseToolTest, InvalidTarget) {
  RunWithExpectedError(FROM_HERE, R"({ "target": {} })");
}

}  // namespace ai_chat
