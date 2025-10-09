// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/scroll_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "chrome/browser/actor/tools/scroll_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class ScrollToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<ScrollTool>(mock_task_provider_.get());
  }

  std::string CreateToolInputJson(const base::Value::Dict& target_dict,
                                  const std::string& direction = "down",
                                  double distance = 100.0) {
    base::Value::Dict dict;
    dict.Set("direction", direction);
    dict.Set("distance", distance);
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  optimization_guide::proto::Action VerifySuccess(
      const std::string& input_json,
      optimization_guide::proto::ScrollAction::ScrollDirection
          expected_direction,
      float expected_distance) {
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "Scroll");
    // Verify proto action properties
    EXPECT_TRUE(action.has_scroll());
    const auto& scroll_action = action.scroll();
    EXPECT_EQ(scroll_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(scroll_action.direction(), expected_direction);
    EXPECT_FLOAT_EQ(scroll_action.distance(), expected_distance);

    // Target verification should be handled by the target_test_util methods
    EXPECT_TRUE(scroll_action.has_target());

    // Convert direction enum values
    actor::mojom::ScrollAction::ScrollDirection expected_mojom_direction;
    switch (expected_direction) {
      case optimization_guide::proto::ScrollAction::LEFT:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kLeft;
        break;
      case optimization_guide::proto::ScrollAction::RIGHT:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kRight;
        break;
      case optimization_guide::proto::ScrollAction::UP:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kUp;
        break;
      case optimization_guide::proto::ScrollAction::DOWN:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kDown;
        break;
      default:
        NOTREACHED() << "Unknown direction: " << expected_direction;
    }

    auto* scroll_request =
        static_cast<actor::ScrollToolRequest*>(tool_request.get());

    // Verify mojom action properties
    auto mojo_action = scroll_request->ToMojoToolAction();
    EXPECT_TRUE(mojo_action->is_scroll());
    const auto& mojom_scroll = mojo_action->get_scroll();
    EXPECT_FLOAT_EQ(mojom_scroll->distance, expected_distance);
    EXPECT_EQ(mojom_scroll->direction, expected_mojom_direction);

    return action;
  }
};

TEST_F(ScrollToolTest, ValidInputWithDocumentTargetDown) {
  auto target_dict = target_test_util::GetDocumentTargetDict("doc123");
  std::string input_json = CreateToolInputJson(target_dict, "down", 150.0);

  auto action = VerifySuccess(
      input_json, optimization_guide::proto::ScrollAction::DOWN, 150.0f);

  const auto& target = action.scroll().target();
  target_test_util::VerifyDocumentTarget(target, "doc123");
}

TEST_F(ScrollToolTest, ValidInputWithContentNodeDown) {
  auto target_dict = target_test_util::GetContentNodeTargetDict(42, "doc123");
  std::string input_json = CreateToolInputJson(target_dict, "down", 150.0);

  auto action = VerifySuccess(
      input_json, optimization_guide::proto::ScrollAction::DOWN, 150.0f);

  const auto& target = action.scroll().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(ScrollToolTest, ValidInputWithCoordinatesUp) {
  auto target_dict = target_test_util::GetCoordinateTargetDict(100, 200);
  std::string input_json = CreateToolInputJson(target_dict, "up", 250.5);

  auto action = VerifySuccess(
      input_json, optimization_guide::proto::ScrollAction::UP, 250.5f);

  const auto& target = action.scroll().target();
  target_test_util::VerifyCoordinateTarget(target, 100, 200);
}

TEST_F(ScrollToolTest, ValidInputLeftDirection) {
  auto target_dict = target_test_util::GetContentNodeTargetDict(99, "mydoc");
  std::string input_json = CreateToolInputJson(target_dict, "left", 50.0);

  auto action = VerifySuccess(
      input_json, optimization_guide::proto::ScrollAction::LEFT, 50.0f);

  const auto& target = action.scroll().target();
  target_test_util::VerifyContentNodeTarget(target, 99, "mydoc");
}

TEST_F(ScrollToolTest, ValidInputRightDirection) {
  auto target_dict = target_test_util::GetCoordinateTargetDict(250.7, 350.3);
  std::string input_json = CreateToolInputJson(target_dict, "right", 75.25);

  auto action = VerifySuccess(
      input_json, optimization_guide::proto::ScrollAction::RIGHT, 75.25f);

  const auto& target = action.scroll().target();
  target_test_util::VerifyCoordinateTarget(target, 250, 350);
}

TEST_F(ScrollToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(ScrollToolTest, MissingDirection) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("distance", 100.0);
  // Note: No direction intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(ScrollToolTest, InvalidDirection) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "diagonal");  // Invalid direction
  dict.Set("distance", 100.0);

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(ScrollToolTest, MissingDistance) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "down");
  // Note: No distance intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(ScrollToolTest, NegativeDistance) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "down");
  dict.Set("distance", -50.0);  // Negative distance

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(ScrollToolTest, ZeroDistance) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "down");
  dict.Set("distance", 0.0);  // Zero distance

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(ScrollToolTest, MissingTargetObject) {
  base::Value::Dict dict;
  dict.Set("direction", "down");
  dict.Set("distance", 100.0);
  // Note: No target intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(ScrollToolTest, InvalidTargetValidation) {
  // Verify the tool properly handles invalid targets
  // and returns appropriate error messages from target_util
  RunWithExpectedError(FROM_HERE, R"({
    "direction": "down",
    "distance": 100.0,
    "target": {}
  })");
}

}  // namespace ai_chat
