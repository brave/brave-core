// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/drag_and_release_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/drag_and_release_tool_request.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class DragAndReleaseToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<DragAndReleaseTool>(mock_task_provider_.get());
  }

  // Creates a valid drag and release JSON with the given source and target
  std::string CreateToolInputJson(const base::Value::Dict& from_target,
                                  const base::Value::Dict& to_target) {
    base::Value::Dict dict;
    dict.Set("from", from_target.Clone());
    dict.Set("to", to_target.Clone());

    return *base::WriteJson(dict);
  }

  // Verify drag and release action and tool request creation
  optimization_guide::proto::Action VerifySuccess(
      const std::string& input_json) {
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "DragAndRelease");

    // Verify proto action
    EXPECT_TRUE(action.has_drag_and_release());

    const auto& drag_action = action.drag_and_release();
    EXPECT_EQ(drag_action.tab_id(), test_tab_handle_.raw_value());

    // Both targets should be present
    EXPECT_TRUE(drag_action.has_from_target());
    EXPECT_TRUE(drag_action.has_to_target());

    return action;
  }
};

// Test valid drag from content node to content node
TEST_F(DragAndReleaseToolTest, DragFromContentNodeToContentNode) {
  // Use standard content node targets from target_test_util
  auto from_target =
      target_test_util::GetContentNodeTargetDict(42, "source_doc");
  auto to_target = target_test_util::GetContentNodeTargetDict(99, "dest_doc");
  std::string input_json = CreateToolInputJson(from_target, to_target);

  auto action = VerifySuccess(input_json);

  // Verify from target
  const auto& from_target_proto = action.drag_and_release().from_target();
  target_test_util::VerifyContentNodeTarget(from_target_proto, 42,
                                            "source_doc");

  // Verify to target
  const auto& to_target_proto = action.drag_and_release().to_target();
  target_test_util::VerifyContentNodeTarget(to_target_proto, 99, "dest_doc");
}

// Test valid drag from content node to coordinates
TEST_F(DragAndReleaseToolTest, DragFromContentNodeToCoordinates) {
  auto from_target =
      target_test_util::GetContentNodeTargetDict(42, "source_doc");
  auto to_target = target_test_util::GetCoordinateTargetDict(250, 350);
  std::string input_json = CreateToolInputJson(from_target, to_target);

  auto action = VerifySuccess(input_json);

  // Verify from target
  const auto& from_target_proto = action.drag_and_release().from_target();
  target_test_util::VerifyContentNodeTarget(from_target_proto, 42,
                                            "source_doc");

  // Verify to target
  const auto& to_target_proto = action.drag_and_release().to_target();
  target_test_util::VerifyCoordinateTarget(to_target_proto, 250, 350);
}

// Test valid drag from coordinates to coordinates
TEST_F(DragAndReleaseToolTest, DragFromCoordinatesToCoordinates) {
  auto from_target = target_test_util::GetCoordinateTargetDict(100, 200);
  auto to_target = target_test_util::GetCoordinateTargetDict(250, 350);
  std::string input_json = CreateToolInputJson(from_target, to_target);

  auto action = VerifySuccess(input_json);

  // Verify from target
  const auto& from_target_proto = action.drag_and_release().from_target();
  target_test_util::VerifyCoordinateTarget(from_target_proto, 100, 200);

  // Verify to target
  const auto& to_target_proto = action.drag_and_release().to_target();
  target_test_util::VerifyCoordinateTarget(to_target_proto, 250, 350);
}

TEST_F(DragAndReleaseToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "invalid json");
}

TEST_F(DragAndReleaseToolTest, MissingFromTarget) {
  auto to_target = target_test_util::GetContentNodeTargetDict();
  std::string input_json =
      absl::StrFormat(R"({"to": %s})", base::WriteJson(to_target).value());

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(DragAndReleaseToolTest, MissingToTarget) {
  auto from_target = target_test_util::GetContentNodeTargetDict();
  std::string input_json =
      absl::StrFormat(R"({"from": %s})", base::WriteJson(from_target).value());

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(DragAndReleaseToolTest, InvalidFromTarget) {
  auto to_target = target_test_util::GetContentNodeTargetDict();
  std::string input_json = absl::StrFormat(R"({"to": %s, "from": {}})",
                                           base::WriteJson(to_target).value());

  RunWithExpectedError(FROM_HERE, input_json);
}

TEST_F(DragAndReleaseToolTest, InvalidToTarget) {
  auto from_target = target_test_util::GetContentNodeTargetDict();
  std::string input_json = absl::StrFormat(
      R"({"from": %s, "to": {}})", base::WriteJson(from_target).value());

  RunWithExpectedError(FROM_HERE, input_json);
}

}  // namespace ai_chat
