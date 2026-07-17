// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_tool_provider.h"

#include <utility>

#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"
#include "brave/components/ai_chat/core/common/features.h"

namespace ai_chat {

WorkspaceToolProvider::WorkspaceToolProvider(
    base::WeakPtr<WorkspaceService> service)
    : service_(std::move(service)) {}

WorkspaceToolProvider::~WorkspaceToolProvider() = default;

void WorkspaceToolProvider::OnConversationHandlerReady(
    const std::string& conversation_uuid) {
  conversation_id_ = conversation_uuid;
  tools_ = BuildWorkspaceTools(service_, conversation_id_);
}

std::vector<base::WeakPtr<Tool>> WorkspaceToolProvider::GetTools() {
  if (!features::IsAIChatWorkspaceToolsEnabled() || !service_ ||
      conversation_id_.empty() || !service_->HasWorkspace(conversation_id_)) {
    return {};
  }

  std::vector<base::WeakPtr<Tool>> tools;
  tools.reserve(tools_.size());
  for (const auto& tool : tools_) {
    tools.push_back(tool->GetWeakPtr());
  }
  return tools;
}

}  // namespace ai_chat
