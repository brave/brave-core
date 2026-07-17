// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WORKSPACE_TOOL_PROVIDER_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_WORKSPACE_TOOL_PROVIDER_FACTORY_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider_factory.h"

namespace ai_chat {

class ToolProvider;
class WorkspaceService;

// Creates a WorkspaceToolProvider per conversation, sharing the profile's
// WorkspaceService (which owns the per-conversation selected folder + file
// I/O). AIChatServiceFactory depends on WorkspaceServiceFactory so the service
// outlives the providers.
class WorkspaceToolProviderFactory : public ToolProviderFactory {
 public:
  explicit WorkspaceToolProviderFactory(WorkspaceService* service);
  ~WorkspaceToolProviderFactory() override;

  WorkspaceToolProviderFactory(const WorkspaceToolProviderFactory&) = delete;
  WorkspaceToolProviderFactory& operator=(const WorkspaceToolProviderFactory&) =
      delete;

  // ToolProviderFactory:
  std::unique_ptr<ToolProvider> CreateToolProvider() override;

 private:
  raw_ptr<WorkspaceService> service_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WORKSPACE_TOOL_PROVIDER_FACTORY_H_
