// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_
#define BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

class CodeExecutionTool;
class ConversationHandler;
class RequestURLTool;
class TabManagementTool;

// Implementation of ToolProvider that provides browser-specific
// tools for conversations.
// It is responsible for grouping browser action tasks (a set of tabs)
// that the tools for a conversation perform actions on.
class BrowserToolProvider : public ToolProvider {
 public:
  explicit BrowserToolProvider(content::BrowserContext* browser_context);

  ~BrowserToolProvider() override;

  BrowserToolProvider(const BrowserToolProvider&) = delete;
  BrowserToolProvider& operator=(const BrowserToolProvider&) = delete;

  // ToolProvider implementation
  std::vector<base::WeakPtr<Tool>> GetTools() override;
  void OnBoundToConversationHandler(ConversationHandler* handler) override;

 private:
  void CreateTools(content::BrowserContext* browser_context);
  void AttachURL(GURL url,
                 std::string title,
                 base::OnceCallback<void(std::string)> on_complete);

  // Browser-specific tools owned by this provider
  std::unique_ptr<CodeExecutionTool> code_execution_tool_;
  std::unique_ptr<RequestURLTool> request_url_tool_;
#if BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL)
  std::unique_ptr<TabManagementTool> tab_management_tool_;
#endif

  raw_ptr<content::BrowserContext> browser_context_ = nullptr;
  raw_ptr<ConversationHandler> conversation_handler_ = nullptr;

  base::WeakPtrFactory<BrowserToolProvider> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_
