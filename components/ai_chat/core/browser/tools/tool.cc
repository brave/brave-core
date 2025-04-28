// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool.h"

#include "base/functional/callback.h"
#include "base/logging.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

std::string_view Tool::Type() const {
  return "function";
}

std::optional<std::string> Tool::InputProperties() const {
  return std::nullopt;
}

std::optional<std::vector<std::string>> Tool::RequiredProperties() const {
  return std::nullopt;
}

bool Tool::IsContentAssociationRequired() const {
  return false;
}

bool Tool::IsAgentTool() const {
  return false;
}

bool Tool::IsSupportedByModel(const mojom::Model& model) const {
  // Implementors should add any extra checks in an override.
  return model.supports_tools;
}

bool Tool::RequiresUserInteractionBeforeHandling() const {
  return false;
}

std::optional<base::Value::Dict> Tool::ExtraParams() const {
  return std::nullopt;
}

void Tool::UseTool(const std::string& input_json,
                   Tool::UseToolCallback callback) {
  CHECK(false) << "UseTool called but not implemented";
}

}  // namespace ai_chat
