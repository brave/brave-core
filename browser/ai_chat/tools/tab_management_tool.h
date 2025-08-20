// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "components/tabs/public/tab_group.h"
#include "components/tabs/public/tab_interface.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

class BrowserWindowInterface;
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
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool IsAgentTool() const override;
  bool RequiresUserInteractionBeforeHandling() const override;
  void UseTool(const std::string& input_json,
               std::optional<base::Value> client_data,
               UseToolCallback callback) override;

 private:
  void OnParseJson(UseToolCallback callback,
                   data_decoder::DataDecoder::ValueOrError result);

  // Action handlers
  void HandleListTabs(UseToolCallback callback);
  void HandleMoveTabs(UseToolCallback callback,
                      const base::Value::Dict& params);
  void HandleReorderTabs(UseToolCallback callback,
                         const base::Value::Dict& params);
  void HandleCloseTabs(UseToolCallback callback,
                       const base::Value::Dict& params);
  void HandleCreateGroup(UseToolCallback callback,
                         const base::Value::Dict& params);
  void HandleUpdateGroup(UseToolCallback callback,
                         const base::Value::Dict& params);
  void HandleRemoveFromGroup(UseToolCallback callback,
                             const base::Value::Dict& params);

  // Move operation branches
  void HandleMoveGroup(UseToolCallback callback,
                       const std::string& group_id,
                       std::optional<int> window_id,
                       std::optional<int> index);
  void HandleMoveIndividualTabs(UseToolCallback callback,
                                const std::vector<int>& tab_handles,
                                std::optional<int> window_id,
                                std::optional<int> index,
                                const std::string* group_id,
                                bool add_to_end);

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
  base::Value::Dict GenerateTabList() const;

  // Send a result, adding the current tab list to it
  void SendResultWithTabList(UseToolCallback callback,
                             base::Value::Dict result);

  // Helper to post a task to send a result with the current tab list, since
  // some window operations are delayed until the next task.
  void PostTaskSendResultWithTabList(
      UseToolCallback callback,
      base::Value::Dict result,
      std::optional<tabs::TabHandle> active_moved_tab = std::nullopt);

  // Helper functions for working with TabHandles
  tabs::TabInterface* GetTabFromHandle(tabs::TabHandle handle);
  std::vector<tabs::TabInterface*> GetTabsFromHandles(
      const std::vector<int>& handles);

  // Profile with which to restrict all window and tab operations
  raw_ptr<Profile> profile_ = nullptr;

  // Conversation-level permission state
  bool user_has_granted_permission_ = false;

  base::WeakPtrFactory<TabManagementTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_MANAGEMENT_TOOL_H_
