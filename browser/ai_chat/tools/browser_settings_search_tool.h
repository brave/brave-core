// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_SEARCH_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_SEARCH_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

// Lets the assistant discover which browser settings it can ask about.
//
// This tool only ever returns preference paths from the static allowlist,
// never the user's values. Actually reading a value is the job of
// BrowserSettingsValueTool, which asks for permission separately on every
// call.
//
// Because the results are the same for every user, a single grant covers the
// rest of the conversation.
class BrowserSettingsSearchTool : public Tool {
 public:
  BrowserSettingsSearchTool();
  ~BrowserSettingsSearchTool() override;

  BrowserSettingsSearchTool(const BrowserSettingsSearchTool&) = delete;
  BrowserSettingsSearchTool& operator=(const BrowserSettingsSearchTool&) =
      delete;

  // Tool:
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  void UserPermissionGranted(const std::string& tool_use_id) override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  bool user_has_granted_permission_ = false;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_SEARCH_TOOL_H_
