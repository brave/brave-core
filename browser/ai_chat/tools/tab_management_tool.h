// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "components/tabs/public/tab_interface.h"

class Profile;
class BrowserWindowInterface;
class TabGroup;

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
  void HandleMoveTabs(UseToolCallback callback, const base::DictValue& params);
  void HandleMoveGroup(UseToolCallback callback, const base::DictValue& params);
  void HandleCreateGroup(UseToolCallback callback,
                         const base::DictValue& params);
  void HandleUpdateGroup(UseToolCallback callback,
                         const base::DictValue& params);
  void HandleRemoveFromGroup(UseToolCallback callback,
                             const base::DictValue& params);

  // Helper to take window param and validate then find or create a window
  BrowserWindowInterface* FindOrCreateTargetWindow(std::optional<int> window_id,
                                                   std::string* error_out,
                                                   bool* did_create_window_out);
  // Provides the window and the group after validation from an incoming group
  // id param
  BrowserWindowInterface* FindWindowWithGroup(const std::string& group_id,
                                              TabGroup** group_out);

  // Validate that that index is valid for the target window tab strip
  bool ValidateMoveTarget(BrowserWindowInterface* target_window,
                          std::optional<int> index,
                          std::string* error) const;

  // Helper to generate tab list that can be reused by all handlers
  base::DictValue GenerateTabList() const;

  // Send a result, adding the current tab list to it
  void SendResultWithTabList(UseToolCallback callback, base::DictValue result);

  // Helper to post a task to send a result with the current tab list, since
  // some window operations are delayed until the next task.
  void PostTaskSendResultWithTabList(
      UseToolCallback callback,
      base::DictValue result,
      std::optional<tabs::TabHandle> active_moved_tab = std::nullopt);

  // Conversation-level permission state
  bool user_has_granted_permission_ = false;

  // Profile with which to restrict all window and tab operations. Usually
  // owns us via AIChatService as ProfileKeyedService.
  raw_ptr<Profile> profile_ = nullptr;

  base::WeakPtrFactory<TabManagementTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_
