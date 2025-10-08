// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/type_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "chrome/browser/actor/tools/type_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class TypeToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<TypeTool>(mock_task_provider_.get());
  }

  // Creates a valid type JSON with the given target and type properties
  std::string CreateToolInputJson(const base::Value::Dict& target_dict,
                                  const std::string& text = "test text",
                                  const std::string& mode = "replace",
                                  bool follow_by_enter = false) {
    base::Value::Dict dict;
    dict.Set("text", text);
    dict.Set("mode", mode);
    dict.Set("follow_by_enter", follow_by_enter);
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  std::string CreateInvalidTargetJson(const std::string& target_content) {
    return R"({
      "text": "test text",
      "mode": "replace",
      "follow_by_enter": false,
      "target": )" +
           target_content + R"(
    })";
  }

  // Verify type action properties and conversions
  optimization_guide::proto::Action VerifySuccess(
      const std::string& input_json,
      const std::string& expected_text,
      bool expected_follow_by_enter,
      optimization_guide::proto::TypeAction::TypeMode expected_mode) {
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "Type");

    EXPECT_TRUE(action.has_type());

    const auto& type_action = action.type();
    EXPECT_EQ(type_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(type_action.text(), expected_text);
    EXPECT_EQ(type_action.follow_by_enter(), expected_follow_by_enter);
    EXPECT_EQ(type_action.mode(), expected_mode);

    // Target verification should be handled by the target_test_util methods
    EXPECT_TRUE(type_action.has_target());

    auto* type_request =
        static_cast<actor::TypeToolRequest*>(tool_request.get());
    EXPECT_EQ(type_request->text, expected_text);
    EXPECT_EQ(type_request->follow_by_enter, expected_follow_by_enter);

    // Verify mode
    actor::TypeToolRequest::Mode expected_actor_mode;
    switch (expected_mode) {
      case optimization_guide::proto::TypeAction::DELETE_EXISTING:
        expected_actor_mode = actor::TypeToolRequest::Mode::kReplace;
        break;
      case optimization_guide::proto::TypeAction::PREPEND:
        expected_actor_mode = actor::TypeToolRequest::Mode::kPrepend;
        break;
      case optimization_guide::proto::TypeAction::APPEND:
        expected_actor_mode = actor::TypeToolRequest::Mode::kAppend;
        break;
      default:
        NOTREACHED() << "Unknown mode: " << expected_mode;
    }
    EXPECT_EQ(type_request->mode, expected_actor_mode);

    // Verify mojom action properties
    auto mojo_action = type_request->ToMojoToolAction();
    EXPECT_TRUE(mojo_action->is_type());
    const auto& mojom_type = mojo_action->get_type();
    EXPECT_EQ(mojom_type->text, expected_text);
    EXPECT_EQ(mojom_type->follow_by_enter, expected_follow_by_enter);

    // Verify mode
    actor::mojom::TypeAction::Mode expected_mojom_mode;
    switch (expected_actor_mode) {
      case actor::TypeToolRequest::Mode::kReplace:
        expected_mojom_mode = actor::mojom::TypeAction::Mode::kDeleteExisting;
        break;
      case actor::TypeToolRequest::Mode::kPrepend:
        expected_mojom_mode = actor::mojom::TypeAction::Mode::kPrepend;
        break;
      case actor::TypeToolRequest::Mode::kAppend:
        expected_mojom_mode = actor::mojom::TypeAction::Mode::kAppend;
        break;
    }
    EXPECT_EQ(mojom_type->mode, expected_mojom_mode);

    return action;
  }
};

TEST_F(TypeToolTest, ValidInputWithContentNode) {
  // Use standard content node target from target_test_util
  auto target_dict = target_test_util::GetContentNodeTargetDict(42, "doc123");
  std::string input_json =
      CreateToolInputJson(target_dict, "Hello World", "replace", true);

  auto action =
      VerifySuccess(input_json, "Hello World", true,
                    optimization_guide::proto::TypeAction::DELETE_EXISTING);

  // Verify target separately using target_test_util
  const auto& target = action.type().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(TypeToolTest, ValidInputWithCoordinates) {
  // Use standard coordinate target from target_test_util
  auto target_dict = target_test_util::GetCoordinateTargetDict(100, 200);
  std::string input_json =
      CreateToolInputJson(target_dict, "Append text", "append", false);

  auto action = VerifySuccess(input_json, "Append text", false,
                              optimization_guide::proto::TypeAction::APPEND);

  // Verify target separately using target_test_util
  const auto& target = action.type().target();
  target_test_util::VerifyCoordinateTarget(target, 100, 200);
}

TEST_F(TypeToolTest, ValidInputPrependMode) {
  // Use custom content node target with specific values
  auto target_dict = target_test_util::GetContentNodeTargetDict(99, "mydoc");
  std::string input_json =
      CreateToolInputJson(target_dict, "Prepend: ", "prepend", false);

  auto action = VerifySuccess(input_json, "Prepend: ", false,
                              optimization_guide::proto::TypeAction::PREPEND);

  // Verify target separately using target_test_util
  const auto& target = action.type().target();
  target_test_util::VerifyContentNodeTarget(target, 99, "mydoc");
}

TEST_F(TypeToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(TypeToolTest, MissingText) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("mode", "replace");
  dict.Set("follow_by_enter", false);
  // Note: No text intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(TypeToolTest, MissingMode) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("text", "test text");
  dict.Set("follow_by_enter", false);
  // Note: No mode intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(TypeToolTest, InvalidMode) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("text", "test text");
  dict.Set("mode", "invalid_mode");
  dict.Set("follow_by_enter", false);

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(TypeToolTest, MissingFollowByEnter) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("text", "test text");
  dict.Set("mode", "replace");
  // Note: No follow_by_enter intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(TypeToolTest, MissingTarget) {
  std::string input_json = R"({
    "text": "test text",
    "mode": "replace",
    "follow_by_enter": false
  })";

  RunWithExpectedError(FROM_HERE, input_json);
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(TypeToolTest, InvalidTarget) {
  // Verify the tool properly handles invalid targets
  // and returns appropriate error messages from target_util
  RunWithExpectedError(FROM_HERE, R"({
      "text": "test text",
      "mode": "replace",
      "follow_by_enter": false,
      "target": {}
    })");
}

}  // namespace ai_chat
