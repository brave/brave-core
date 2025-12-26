// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_
#define BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

class CodeExecutionTool;
class TabManagementTool;

// Implementation of ToolProvider that provides browser-specific
// tools for conversations.
// It is responsible for grouping browser action tasks (a set of tabs)
// that the tools for a conversation perform actions on.
class BrowserToolProvider : public ToolProvider {
 public:
  explicit BrowserToolProvider(Profile* profile);

  ~BrowserToolProvider() override;

  BrowserToolProvider(const BrowserToolProvider&) = delete;
  BrowserToolProvider& operator=(const BrowserToolProvider&) = delete;

  // ToolProvider implementation
  std::vector<base::WeakPtr<Tool>> GetTools() override;

 private:
  void CreateTools(content::BrowserContext* browser_context);

  // Browser-specific tools owned by this provider
  std::unique_ptr<CodeExecutionTool> code_execution_tool_;
#if BUILDFLAG(ENABLE_TAB_MANAGEMENT_TOOL)
  std::unique_ptr<TabManagementTool> tab_management_tool_ = nullptr;
#endif
  raw_ptr<Profile> profile_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_
