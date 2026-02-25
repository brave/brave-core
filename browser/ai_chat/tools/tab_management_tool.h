// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/tools/tool.h"

class Profile;

namespace ai_chat {

class TabManagementTool : public Tool {
 public:
  explicit TabManagementTool(Profile* profile);
  ~TabManagementTool() override;

  TabManagementTool(const TabManagementTool&) = delete;
  TabManagementTool& operator=(const TabManagementTool&) = delete;

  // Tool implementation
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool IsAgentTool() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  void UserPermissionGranted(const std::string& tool_use_id) override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  // Action handlers
  void HandleListTabs(UseToolCallback callback);

  // Helper to generate tab list that can be reused by all handlers
  base::Value::Dict GenerateTabList() const;
  // Profile with which to restrict all window and tab operations
  raw_ptr<Profile> profile_ = nullptr;

  // Conversation-level permission state
  bool user_has_granted_permission_ = false;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_
