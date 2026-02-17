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
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tab_groups/tab_group_color.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "components/tabs/public/tab_group.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

static_assert(BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL));
static_assert(!BUILDFLAG(IS_ANDROID));

namespace ai_chat {

namespace {

std::string GetTabGroupColorString(tab_groups::TabGroupColorId color) {
  switch (color) {
    case tab_groups::TabGroupColorId::kGrey:
      return "grey";
    case tab_groups::TabGroupColorId::kBlue:
      return "blue";
    case tab_groups::TabGroupColorId::kRed:
      return "red";
    case tab_groups::TabGroupColorId::kYellow:
      return "yellow";
    case tab_groups::TabGroupColorId::kGreen:
      return "green";
    case tab_groups::TabGroupColorId::kPink:
      return "pink";
    case tab_groups::TabGroupColorId::kPurple:
      return "purple";
    case tab_groups::TabGroupColorId::kCyan:
      return "cyan";
    case tab_groups::TabGroupColorId::kOrange:
      return "orange";
    default:
      return "unknown";
  }
}

}  // namespace

TabManagementTool::TabManagementTool(Profile* profile) : profile_(profile) {}

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

  if (*action == "list") {
    HandleListTabs(std::move(callback));
  } else {
    std::move(callback).Run(CreateContentBlocksForText(
        "Invalid action. Must be one of: list, move, close, "
        "create_group, update_group, remove_from_group"));
  }
}

base::Value::Dict TabManagementTool::GenerateTabList() const {
  base::Value::Dict result;
  base::Value::List windows;

  // Iterate through all browser windows for this profile
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    if (browser->GetProfile() != profile_) {
      continue;
    }

    base::Value::Dict window_info;
    window_info.Set("window_id", browser->GetSessionID().id());
    window_info.Set("is_active", browser->IsActive());

    TabStripModel* tab_strip = browser->GetTabStripModel();
    if (!tab_strip) {
      continue;
    }

    base::Value::List tabs;
    base::Value::Dict groups;

    // First, collect group information
    if (tab_strip->SupportsTabGroups() && tab_strip->group_model()) {
      for (const auto& group_id : tab_strip->group_model()->ListTabGroups()) {
        const TabGroup* group = tab_strip->group_model()->GetTabGroup(group_id);
        if (group) {
          base::Value::Dict group_info;
          const auto* visual_data = group->visual_data();
          group_info.Set("title", base::UTF16ToUTF8(visual_data->title()));
          group_info.Set("color", GetTabGroupColorString(visual_data->color()));
          group_info.Set("is_collapsed", tab_strip->IsGroupCollapsed(group_id));

          groups.Set(group_id.ToString(), std::move(group_info));
        }
      }
    }

    // Collect tab information
    for (int i = 0; i < tab_strip->count(); ++i) {
      tabs::TabInterface* tab = tab_strip->GetTabAtIndex(i);
      if (!tab) {
        continue;
      }

      content::WebContents* web_contents = tab->GetContents();

      base::Value::Dict tab_info;
      // Use TabHandle instead of extension tab ID
      tab_info.Set("tab_id", tab->GetHandle().raw_value());
      tab_info.Set("index", i);
      tab_info.Set("url", web_contents->GetURL().spec());
      tab_info.Set("title", base::UTF16ToUTF8(web_contents->GetTitle()));
      tab_info.Set("is_active", i == tab_strip->active_index());
      tab_info.Set("is_pinned", tab_strip->IsTabPinned(i));

      // Add group information if tab is in a group
      auto group_id = tab_strip->GetTabGroupForTab(i);
      if (group_id.has_value()) {
        tab_info.Set("group_id", group_id->ToString());
      }

      tabs.Append(std::move(tab_info));
    }

    window_info.Set("tabs", std::move(tabs));
    window_info.Set("groups", std::move(groups));
    window_info.Set("active_tab_index", tab_strip->active_index());

    windows.Append(std::move(window_info));
  }

  result.Set("windows", std::move(windows));
  return result;
}

void TabManagementTool::HandleListTabs(UseToolCallback callback) {
  base::Value::Dict result = GenerateTabList();

  std::string json_output;
  base::JSONWriter::WriteWithOptions(
      result, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_output);

  std::move(callback).Run(CreateContentBlocksForText(json_output));
}
}  // namespace ai_chat
