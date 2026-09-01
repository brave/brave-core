// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_VALUE_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_VALUE_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

class PrefService;

namespace ai_chat {

namespace internal {

// Serializes the values of `setting_ids` (preference paths) into the JSON
// payload returned by BrowserSettingsValueTool. Exposed for unit tests.
std::string BuildSettingsValueJson(const std::vector<std::string>& setting_ids,
                                   const PrefService& prefs);

}  // namespace internal

// Reads the user's current value for allowlisted browser preferences.
//
// Unlike BrowserSettingsSearchTool, the output of this tool is information
// about the user, so it asks for permission on *every* call rather than once
// per conversation. That is deliberate: a single approval must not become a
// standing licence to enumerate the user's configuration.
class BrowserSettingsValueTool : public Tool {
 public:
  explicit BrowserSettingsValueTool(PrefService* prefs);
  ~BrowserSettingsValueTool() override;

  BrowserSettingsValueTool(const BrowserSettingsValueTool&) = delete;
  BrowserSettingsValueTool& operator=(const BrowserSettingsValueTool&) = delete;

  // Tool:
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  raw_ptr<PrefService> prefs_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_VALUE_TOOL_H_
