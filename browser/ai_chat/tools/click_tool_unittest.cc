// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/click_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/click_tool_request.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class ClickToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<ClickTool>(mock_task_provider_.get());
  }

  std::string CreateToolInputJson(const base::Value::Dict& target_dict,
                                  const std::string& click_type = "left",
                                  const std::string& click_count = "single") {
    base::Value::Dict dict;
    dict.Set("click_type", click_type);
    dict.Set("click_count", click_count);
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  optimization_guide::proto::Action VerifySuccess(
      const std::string& input_json,
      optimization_guide::proto::ClickAction::ClickType expected_click_type =
          optimization_guide::proto::ClickAction::LEFT,
      optimization_guide::proto::ClickAction::ClickCount expected_click_count =
          optimization_guide::proto::ClickAction::SINGLE) {
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "Click");

    EXPECT_TRUE(action.has_click());

    const auto& click_action = action.click();
    EXPECT_EQ(click_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(click_action.click_type(), expected_click_type);
    EXPECT_EQ(click_action.click_count(), expected_click_count);

    // Target verification should be handled by the target_test_util methods
    // in each test.
    EXPECT_TRUE(click_action.has_target());

    return action;
  }
};

TEST_F(ClickToolTest, ValidInput) {
  auto target_dict = target_test_util::GetContentNodeTargetDict(42, "doc123");
  std::string input_json = CreateToolInputJson(target_dict);

  auto action =
      VerifySuccess(input_json, optimization_guide::proto::ClickAction::LEFT,
                    optimization_guide::proto::ClickAction::SINGLE);

  // Verify target separately using target_test_util
  const auto target = action.click().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(ClickToolTest, ValidInputRightDoubleClick) {
  // Use coordinates
  auto target_dict = target_test_util::GetCoordinateTargetDict(99, 200);
  std::string input_json = CreateToolInputJson(target_dict, "right", "double");

  auto action =
      VerifySuccess(input_json, optimization_guide::proto::ClickAction::RIGHT,
                    optimization_guide::proto::ClickAction::DOUBLE);

  // Verify target separately
  const auto target = action.click().target();
  target_test_util::VerifyCoordinateTarget(target, 99, 200);
}

TEST_F(ClickToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(ClickToolTest, MissingClickType) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("click_count", "single");
  // Note: No click_type intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(ClickToolTest, InvalidClickType) {
  std::string input_json = CreateToolInputJson(
      target_test_util::GetContentNodeTargetDict(), "doesnotexist", "single");

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(ClickToolTest, MissingTarget) {
  std::string input_json = R"({
    "click_type": "left",
    "click_count": "single"
  })";

  RunWithExpectedError(FROM_HERE, input_json);
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(ClickToolTest, InvalidTarget) {
  // Empty target value should fail
  RunWithExpectedError(FROM_HERE, CreateToolInputJson(base::Value::Dict()));
}

}  // namespace ai_chat
