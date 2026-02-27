// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_management_tool.h"

#include <optional>
#include <string>
#include <vector>

#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kValidInputListWithPlan[] =
    R"({"action":"list", "plan":"move things around"})";

constexpr char kValidInputListWithoutPlan[] = R"({"action":"list"})";

constexpr char kValidInputListWithPlanWrongFormat[] =
    R"({"action":"list", "plan": true})";

constexpr char kValidInputListWithEmptyPlan[] =
    R"({"action":"list", "plan": ""})";

std::string ExtractText(const std::vector<mojom::ContentBlockPtr>& blocks) {
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

std::string RunTool(TabManagementTool* tool, const std::string& json) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                         std::vector<mojom::ToolArtifactPtr>>
      future;
  tool->UseTool(json, future.GetCallback());
  return ExtractText(future.Get<std::vector<mojom::ContentBlockPtr>>());
}

mojom::ToolUseEventPtr CreateToolUseEvent(const std::string& json) {
  return mojom::ToolUseEvent::New(mojom::kTabManagementToolName, "1", json,
                                  std::nullopt, std::nullopt, nullptr, false);
}

}  // namespace

TEST(TabManagementToolUnitTest, RequiresUserInteractionBeforeHandling) {
  TabManagementTool tool;

  auto valid_tool_event = CreateToolUseEvent(kValidInputListWithPlan);

  // By default, we should ask for permission via user interaction
  std::variant<bool, mojom::PermissionChallengePtr> result =
      tool.RequiresUserInteractionBeforeHandling(*valid_tool_event);
  ASSERT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(result));
  EXPECT_EQ((std::get<mojom::PermissionChallengePtr>(result))->plan,
            "move things around");

  // If a plan isn't provided, we'll return that we don't need user interaction,
  // since UseTool will fail and tell the AI what's missing.
  result = tool.RequiresUserInteractionBeforeHandling(
      *CreateToolUseEvent(kValidInputListWithoutPlan));
  ASSERT_TRUE(std::holds_alternative<bool>(result));
  EXPECT_FALSE((std::get<bool>(result)));

  // Same for plan is the wrong format
  result = tool.RequiresUserInteractionBeforeHandling(
      *CreateToolUseEvent(kValidInputListWithPlanWrongFormat));
  ASSERT_TRUE(std::holds_alternative<bool>(result));
  EXPECT_FALSE((std::get<bool>(result)));

  // Same for empty plan
  result = tool.RequiresUserInteractionBeforeHandling(
      *CreateToolUseEvent(kValidInputListWithEmptyPlan));
  ASSERT_TRUE(std::holds_alternative<bool>(result));
  EXPECT_FALSE((std::get<bool>(result)));
}

TEST(TabManagementToolUnitTest, UseTool_Permissions) {
  TabManagementTool tool;

  // Not providing a plan should provide the reason during UseTool so that
  // the error is reported to the AI who is given another chance to provide a
  // valid tool call following the directions.
  EXPECT_THAT(RunTool(&tool, kValidInputListWithoutPlan),
              testing::HasSubstr("No plan provided"));
  EXPECT_THAT(RunTool(&tool, kValidInputListWithPlanWrongFormat),
              testing::HasSubstr("No plan provided"));
  EXPECT_THAT(RunTool(&tool, kValidInputListWithEmptyPlan),
              testing::HasSubstr("No plan provided"));

  // Sanity check not providing permission should fail.
  // Without calling UserPermissionGranted, but with a valid input, UseTool
  // should be rejected. This is a safeguard to the caller not calling
  // UserPermissionGranted or RequiresUserInteractionBeforeHandling.
  EXPECT_THAT(RunTool(&tool, kValidInputListWithPlan),
              testing::HasSubstr("Unknown error"));

  // Grant permission and verify we proceed to JSON validation.
  tool.UserPermissionGranted("");
  EXPECT_THAT(RunTool(&tool, "{}"),
              testing::HasSubstr("Missing required 'action' field"));

  // Subsequent calls should not require permission again.
  EXPECT_THAT(RunTool(&tool, "{}"),
              testing::HasSubstr("Missing required 'action' field"));
}

TEST(TabManagementToolUnitTest, JsonAndArgumentValidationErrors) {
  TabManagementTool tool;

  // Grant permission once.
  tool.UserPermissionGranted("");

  // Parse failure (not a dict)
  {
    std::string result = RunTool(&tool, "[]");
    ASSERT_FALSE(result.empty());
    EXPECT_THAT(result, testing::HasSubstr("Failed to parse input JSON"));
  }

  // Missing action
  EXPECT_THAT(RunTool(&tool, "{}"),
              testing::HasSubstr("Missing required 'action' field"));
}

}  // namespace ai_chat
