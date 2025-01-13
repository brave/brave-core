// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool.h"

#include "base/logging.h"
#include "base/functional/callback.h"

namespace ai_chat {

std::string_view Tool::type() const {
  return "function";
}

std::optional<std::string> Tool::GetInputSchemaJson() const {
  return std::nullopt;
}

std::optional<std::vector<std::string>> Tool::required_properties() const {
  return std::nullopt;
}

bool Tool::IsContentAssociationRequired() const {
  return false;
}

bool Tool::IsAgentTool() const {
  return false;
}

bool Tool::RequiresUserInteractionBeforeHandling() const {
  return false;
}

std::optional<base::Value> Tool::extra_params() const {
  return std::nullopt;
}

void Tool::UseTool(
      const std::string& input_json,
      Tool::UseToolCallback callback) {
  DLOG(ERROR) << "UseTool called but not implemented";
  std::move(callback).Run(std::nullopt, 0);
}

}  // namespace ai_chat
