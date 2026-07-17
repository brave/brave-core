// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_TOOL_PROVIDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_TOOL_PROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_tool.h"

namespace ai_chat {

class Tool;
class WorkspaceService;

// Provides the "workspace" local file tools to a single conversation. The tools
// are exposed only when the kAIChatWorkspaceTools feature is on and the
// conversation has a workspace folder open (see WorkspaceService). One instance
// is created per ConversationHandler by WorkspaceToolProviderFactory.
class WorkspaceToolProvider : public ToolProvider {
 public:
  explicit WorkspaceToolProvider(base::WeakPtr<WorkspaceService> service);
  ~WorkspaceToolProvider() override;

  WorkspaceToolProvider(const WorkspaceToolProvider&) = delete;
  WorkspaceToolProvider& operator=(const WorkspaceToolProvider&) = delete;

  // ToolProvider:
  void OnConversationHandlerReady(
      const std::string& conversation_uuid) override;
  std::vector<base::WeakPtr<Tool>> GetTools() override;

 private:
  base::WeakPtr<WorkspaceService> service_;
  std::string conversation_id_;
  std::vector<std::unique_ptr<WorkspaceTool>> tools_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_TOOL_PROVIDER_H_
