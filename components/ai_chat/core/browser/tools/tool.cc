// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool.h"

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

Tool::Tool() = default;
Tool::~Tool() = default;

std::string_view Tool::Type() const {
  return "function";
}

std::optional<base::Value::Dict> Tool::InputProperties() const {
  return std::nullopt;
}

std::optional<std::vector<std::string>> Tool::RequiredProperties() const {
  return std::nullopt;
}

bool Tool::IsAgentTool() const {
  return false;
}

bool Tool::IsSupportedByModel(const mojom::Model& model) const {
  // Implementors should add any extra checks in an override.
  return model.supports_tools;
}

std::variant<bool, mojom::PermissionChallengePtr>
Tool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  return false;
}

void Tool::UserPermissionGranted(const std::string& tool_use_id) {
  // Default: no-op. Tools can override if they need to track permission state.
}

bool Tool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    mojom::ConversationCapability conversation_capability) const {
  return true;
}

std::optional<base::Value::Dict> Tool::ExtraParams() const {
  return std::nullopt;
}

void Tool::UseTool(const std::string& input_json,
                   Tool::UseToolCallback callback) {
  CHECK(false) << "UseTool called but not implemented";
}

}  // namespace ai_chat
