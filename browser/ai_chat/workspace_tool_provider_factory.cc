// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/workspace_tool_provider_factory.h"

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_tool_provider.h"

namespace ai_chat {

WorkspaceToolProviderFactory::WorkspaceToolProviderFactory(
    WorkspaceService* service)
    : service_(service) {}

WorkspaceToolProviderFactory::~WorkspaceToolProviderFactory() = default;

std::unique_ptr<ToolProvider>
WorkspaceToolProviderFactory::CreateToolProvider() {
  return std::make_unique<WorkspaceToolProvider>(
      service_ ? service_->GetWeakPtr() : base::WeakPtr<WorkspaceService>());
}

}  // namespace ai_chat
