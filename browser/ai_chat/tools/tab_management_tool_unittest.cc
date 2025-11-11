// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_management_tool.h"

#include <optional>
#include <string>
#include <vector>

#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

std::string ExtractText(const std::vector<mojom::ContentBlockPtr>& blocks) {
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

std::string RunTool(TabManagementTool* tool,
                    const std::string& json,
                    std::optional<base::Value> client_data = std::nullopt) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> future;
  tool->UseTool(json, std::move(client_data), future.GetCallback());
  return ExtractText(future.Get());
}

}  // namespace

class TabManagementToolUnitTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;  // For DataDecoder tasks
  TestingProfile profile_;
};

TEST_F(TabManagementToolUnitTest, PermissionDeniedAndGrantFlow) {
  TabManagementTool tool(&profile_);

  // Denied path returns user-facing message, does not CHECK.
  {
    std::string result = RunTool(&tool, "{}", base::Value(false));
    ASSERT_FALSE(result.empty());
    EXPECT_THAT(result, testing::HasSubstr("User has not granted permission"));
  }

  // Grant permission and verify we proceed to JSON validation.
  {
    std::string result = RunTool(&tool, "{}", base::Value(true));
    ASSERT_FALSE(result.empty());
    EXPECT_THAT(result, testing::HasSubstr("Missing required 'action' field"));
  }

  // Subsequent calls without client_data should not require permission again.
  EXPECT_THAT(RunTool(&tool, "{}"),
              testing::HasSubstr("Missing required 'action' field"));
}

TEST_F(TabManagementToolUnitTest, JsonAndArgumentValidationErrors) {
  TabManagementTool tool(&profile_);

  // Grant permission once.
  RunTool(&tool, R"({"action":"list"})", base::Value(true));

  // Parse failure (not a dict)
  {
    std::string result = RunTool(&tool, "[]");
    ASSERT_FALSE(result.empty());
    EXPECT_THAT(result, testing::HasSubstr("Failed to parse input JSON"));
  }

  // Missing action
  EXPECT_THAT(RunTool(&tool, "{}"),
              testing::HasSubstr("Missing required 'action' field"));

  // Invalid action
  EXPECT_THAT(RunTool(&tool, R"({"action":"bogus"})"),
              testing::HasSubstr("Invalid action. Must be one of"));

  // Per-action validation
  // move without tab_ids or move_group_id
  EXPECT_THAT(RunTool(&tool, R"({"action":"move"})"),
              testing::HasSubstr("Missing 'tab_ids' array or 'move_group_id'"));

  // close without tab_ids
  EXPECT_THAT(RunTool(&tool, R"({"action":"close"})"),
              testing::HasSubstr(
                  "Missing or empty 'tab_ids' array for close operation"));

  // create_group without tab_ids
  EXPECT_THAT(
      RunTool(&tool, R"({"action":"create_group"})"),
      testing::HasSubstr(
          "Missing or empty 'tab_ids' array for create_group operation"));

  // update_group without group_id
  EXPECT_THAT(
      RunTool(&tool, R"({"action":"update_group"})"),
      testing::HasSubstr("Missing 'group_id' for update_group operation"));

  // remove_from_group without tab_ids
  EXPECT_THAT(
      RunTool(&tool, R"({"action":"remove_from_group"})"),
      testing::HasSubstr(
          "Missing or empty 'tab_ids' array for remove_from_group operation"));
}

// Additional unit tests focusing on validation logic that can be tested without
// a browser. The browser-dependent tests have been consolidated in the browser
// test suite to reduce test execution time. This unit test focuses on parameter
// validation, mutual exclusivity, and error handling.
TEST_F(TabManagementToolUnitTest, MoveParameterValidationAndMutualExclusivity) {
  TabManagementTool tool(&profile_);

  // Grant permission once
  RunTool(&tool, R"({"action":"list"})", base::Value(true));

  // Empty tab_ids array should be treated as missing
  EXPECT_THAT(RunTool(&tool, R"({"action":"move","tab_ids":[]})"),
              testing::HasSubstr("Missing 'tab_ids' array or 'move_group_id'"));

  // Empty move_group_id should be treated as missing
  EXPECT_THAT(RunTool(&tool, R"({"action":"move","move_group_id":""})"),
              testing::HasSubstr("Missing 'tab_ids' array or 'move_group_id'"));

  // move with BOTH tab_ids and move_group_id (mutual exclusivity)
  EXPECT_THAT(
      RunTool(
          &tool,
          R"({"action":"move","tab_ids":[1,2],"move_group_id":"group-abc"})"),
      testing::HasSubstr("Cannot provide both 'tab_ids' and 'move_group_id'"));

  // Both provided but one is empty - strict validation rejects both fields
  // being present
  EXPECT_THAT(
      RunTool(&tool, R"({"action":"move","tab_ids":[1],"move_group_id":""})"),
      testing::HasSubstr("Cannot provide both 'tab_ids' and 'move_group_id'"));

  // Test with non-existent tab handles (fails during tab validation)
  EXPECT_THAT(RunTool(&tool, R"({"action":"move","tab_ids":[99999]})"),
              testing::HasSubstr("No valid tabs found to move"));

  // Test all mutual exclusivity validation cases
  EXPECT_THAT(
      RunTool(&tool, R"({"action":"move","tab_ids":[1],"move_group_id":""})"),
      testing::HasSubstr("Cannot provide both"));  // Strict validation rejects
                                                   // both fields present

  EXPECT_THAT(
      RunTool(&tool,
              R"({"action":"move","tab_ids":[],"move_group_id":"group1"})"),
      testing::HasSubstr("Cannot provide both"));  // Strict validation rejects
                                                   // both fields present

  EXPECT_THAT(RunTool(&tool,
                      R"({
  "action":"move","tab_ids":[1,2,3],"move_group_id":"group1","window_id":-1}
          )"),
              testing::HasSubstr("Cannot provide both"));

  EXPECT_THAT(RunTool(&tool,
                      R"(
              {"action":"move","tab_ids":[1],"move_group_id":"group1","index":0}
                      )"),
              testing::HasSubstr("Cannot provide both"));

  EXPECT_THAT(RunTool(&tool,
                      R"(
        {"action":"move","tab_ids":[1],"group_id":"target-group","window_id":-1}
          )"),
              testing::HasSubstr(
                  "Cannot provide both a target 'group_id' and 'window_id'"));

  // Error responses for nonexistent resources - these don't require actual
  // browser
  EXPECT_THAT(
      RunTool(&tool,
              R"({"action":"move","move_group_id":"nonexistent-group"})"),
      testing::HasSubstr("Group not found"));

  // Test window_id = -1 (which means "create new window") - new window creation
  // deferred until after tab validation
  EXPECT_THAT(
      RunTool(&tool, R"({"action":"move","tab_ids":[1],"window_id":-1})"),
      testing::HasSubstr("No valid tabs found"));  // Tab validation happens
                                                   // before new window creation

  // Test update group with nonexistent group
  EXPECT_THAT(RunTool(&tool,
                      R"({
                          "action":"update_group",
                          "group_id":"nonexistent-group",
                          "group_title":"Test",
                          })"),
              testing::HasSubstr("Group not found"));
}

}  // namespace ai_chat
