// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider.h"

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_chat/code_execution_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {

BrowserToolProvider::BrowserToolProvider(
    content::BrowserContext* browser_context) {
  CreateTools(browser_context);
}

BrowserToolProvider::~BrowserToolProvider() = default;

std::vector<base::WeakPtr<Tool>> BrowserToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tool_ptrs;
  if (code_execution_tool_) {
    tool_ptrs.push_back(code_execution_tool_->GetWeakPtr());
  }
  return tool_ptrs;
}

void BrowserToolProvider::CreateTools(
    content::BrowserContext* browser_context) {
  if (features::IsCodeExecutionToolEnabled()) {
    code_execution_tool_ = std::make_unique<CodeExecutionTool>(browser_context);
  }
}

}  // namespace ai_chat
