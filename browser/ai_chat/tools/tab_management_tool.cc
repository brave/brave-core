// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_management_tool.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

static_assert(BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL));
static_assert(!BUILDFLAG(IS_ANDROID));

namespace ai_chat {

TabManagementTool::TabManagementTool() = default;

TabManagementTool::~TabManagementTool() = default;

std::string_view TabManagementTool::Name() const {
  return mojom::kTabManagementToolName;
}

std::string_view TabManagementTool::Description() const {
  return "Manage browser tabs - list, move, close tabs and manage tab "
         "groups. Only use this tool when the user explicitly requests tab "
         "organization, grouping, moving, or closing tabs. Do not use this "
         "tool proactively or when the user's request can be answered without "
         "modifying their tabs. "
         "This tool can list all open tabs with their window, group, URL and "
         "title information, "
         "move tabs or entire groups between windows or positions,"
         "close tabs, "
         "and create or modify tab groups. Groups are per-window, so make any "
         "moves before grouping. Use window_id=-1 to move tabs/groups to a new "
         "window. "
         "Use move_group_id to move an entire group at once (cannot be "
         "combined with tab_ids in the same request). "
         "When moving tabs, the active tab state is preserved - if you move "
         "the active tab, "
         "it remains active in its new location. "
         "After each operation, the updated tab list is returned. "
         "To move both individual tabs and groups, make separate move "
         "requests. "
         "If possible and you know the operations and IDs ahead of time, "
         "try to make multiple parallel requests to use this tool without "
         "waiting for the answer. Every time this tool needs to be used, if "
         "there has been a user message since the last time it has been used "
         "then the list of tabs should be read again as there has been a gap "
         "in time and the list might have changed significantly.\nThe list "
         "operation should be the first operation as every other operation "
         "requires IDs found in the list of tabs and windows. You must provide "
         "the plan parameter when using the list operation.";
}

std::optional<base::Value::Dict> TabManagementTool::InputProperties() const {
  return CreateInputProperties(
      {{"action", StringProperty("The action to perform",
                                 std::vector<std::string>{
                                     "list", "move", "close", "create_group",
                                     "update_group", "remove_from_group"})},
       {"plan",
        StringProperty("Human readable plan of what the assistant intends to "
                       "do with the list of tabs and with the tab management "
                       "tool. This should be provided during the very first "
                       "list operation in a conversation and allows the user "
                       "to approve or deny the tab management operations.")},
       {"tab_ids",
        ArrayProperty("List of tab IDs to operate on (for move, "
                      "close, create_group, remove_from_group). Cannot be used "
                      "with move_group_id.",
                      IntegerProperty("Tab ID"))},
       {"move_group_id",
        StringProperty("Group ID to move entirely (for move operation). "
                       "Mutually exclusive with tab_ids - use separate "
                       "requests to move both individual tabs and groups.")},
       {"window_id",
        IntegerProperty(
            "Target window ID (for move operation). Use -1 to create a new "
            "window. If group_id is provided, this will be inferred from that "
            "group's window and window_id should not be provided.")},
       {"group_id",
        StringProperty("Target group ID for update_group or move (for move "
                       "operation window ID will be inferred by the group "
                       "windowand is mutually exclusive) or group to update")},
       {"index",
        IntegerProperty("Target index position (for move operations)")},
       {"group_title", StringProperty("Title for new or updated group")},
       {"group_color",
        StringProperty(
            "Color for new or updated group",
            std::vector<std::string>{"grey", "blue", "red", "yellow", "green",
                                     "pink", "purple", "cyan", "orange"})},
       {"add_to_end", BooleanProperty("Add tabs to end of group instead of "
                                      "beginning (for move to group)")}});
}

std::optional<std::vector<std::string>> TabManagementTool::RequiredProperties()
    const {
  return std::vector<std::string>{"action"};
}

bool TabManagementTool::IsAgentTool() const {
  return true;
}

std::variant<bool, mojom::PermissionChallengePtr>
TabManagementTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_has_granted_permission_) {
    return false;
  }
  // Provide PermissionChallenge only if input is valid and we were provided
  // with a plan. If it isn't valid, it will be rejected by UseTool.
  auto input = base::JSONReader::ReadDict(tool_use.arguments_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    return false;
  }

  const auto* plan = input->FindString("plan");
  if (!plan || plan->empty()) {
    return false;
  }

  return mojom::PermissionChallenge::New(std::nullopt, *plan);
}

void TabManagementTool::UserPermissionGranted(const std::string& tool_use_id) {
  user_has_granted_permission_ = true;
}

void TabManagementTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Failed to parse input JSON. Please provide "
                                   "valid JSON with an 'action' field."));
    return;
  }

  const auto& dict = input.value();

  // Verify we have permission
  if (!user_has_granted_permission_) {
    // Report to LLM as to why we couldn't grant permission yet
    auto* plan = dict.FindString("plan");
    if (!plan || plan->empty()) {
      // No plan provided, so we can't grant permission
      std::move(callback).Run(CreateContentBlocksForText(
          "No plan provided which the user will be asked to approve. Provide a "
          "plan for the first use of this tool."));
      return;
    }
    // We shouldn't get here since we expect callers to call
    // RequiresUserInteractionBeforeHandling first and only call UseTool if
    // permission was granted but, just in case, still provide output so the
    // conversation can proceed without running this tool.
    std::move(callback).Run(CreateContentBlocksForText("Unknown error"));
    return;
  }

  CHECK(user_has_granted_permission_);

  const auto* action = dict.FindString("action");

  if (!action) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Missing required 'action' field. Must be one of: list, move, close, "
        "create_group, update_group, remove_from_group"));
    return;
  }

  std::move(callback).Run(
      CreateContentBlocksForText("This tool is not yet implemented"));
}

}  // namespace ai_chat
