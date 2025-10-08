// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/select_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "chrome/browser/actor/tools/select_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class SelectToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<SelectTool>(mock_task_provider_.get());
  }

  // Creates a valid select JSON with the given target and value
  std::string CreateToolInputJson(const base::Value::Dict& target_dict,
                                  const std::string& value = "option1") {
    base::Value::Dict dict;
    dict.Set("value", value);
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  // Verify select action properties and conversions
  optimization_guide::proto::Action VerifySuccess(
      const std::string& input_json,
      const std::string& expected_value) {
    // Verify proto action
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "Select");

    EXPECT_TRUE(action.has_select());

    const auto& select_action = action.select();
    EXPECT_EQ(select_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(select_action.value(), expected_value);

    // Target verification should be handled by the target_test_util methods in
    // each test.
    EXPECT_TRUE(select_action.has_target());

    auto* select_request =
        static_cast<actor::SelectToolRequest*>(tool_request.get());

    // Verify mojom action properties
    auto mojo_action = select_request->ToMojoToolAction();
    EXPECT_TRUE(mojo_action->is_select());
    const auto& mojom_select = mojo_action->get_select();
    EXPECT_EQ(mojom_select->value, expected_value);

    return action;
  }
};

TEST_F(SelectToolTest, ValidInputWithContentNode) {
  // Use standard content node target from target_test_util
  auto target_dict = target_test_util::GetContentNodeTargetDict(42, "doc123");
  std::string input_json = CreateToolInputJson(target_dict, "option1");

  auto action = VerifySuccess(input_json, "option1");

  const auto& target = action.select().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(SelectToolTest, ValidInputWithCoordinates) {
  // Use standard coordinate target from target_test_util
  auto target_dict = target_test_util::GetCoordinateTargetDict(100, 200);
  std::string input_json = CreateToolInputJson(target_dict, "value2");

  // Verify select action properties
  auto action = VerifySuccess(input_json, "value2");

  // Verify target separately using target_test_util
  const auto& target = action.select().target();
  target_test_util::VerifyCoordinateTarget(target, 100, 200);
}

TEST_F(SelectToolTest, ValidInputComplexValue) {
  // Use custom content node target with specific values
  auto target_dict = target_test_util::GetContentNodeTargetDict(99, "mydoc");
  std::string input_json =
      CreateToolInputJson(target_dict, "complex-option-value-123");

  // Verify select properties
  auto action = VerifySuccess(input_json, "complex-option-value-123");

  // Verify target separately
  const auto& target = action.select().target();
  target_test_util::VerifyContentNodeTarget(target, 99, "mydoc");
}

TEST_F(SelectToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(SelectToolTest, MissingValue) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  // Note: No value intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(SelectToolTest, InvalidValueType) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("value", 123);  // Invalid type - should be string

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(SelectToolTest, MissingTarget) {
  std::string input_json = R"({
    "value": "option1"
  })";

  RunWithExpectedError(FROM_HERE, input_json);
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(SelectToolTest, InvalidTarget) {
  // Verify the tool properly handles invalid targets
  // and returns appropriate error messages from target_util
  RunWithExpectedError(FROM_HERE, R"({
      "value": "option1",
      "target": {}
    })");
}

}  // namespace ai_chat
