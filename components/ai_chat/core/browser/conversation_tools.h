// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_TOOLS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_TOOLS_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"

namespace ai_chat {

class TodoTool;

class ConversationToolProvider : public ToolProvider {
 public:
  explicit ConversationToolProvider(base::WeakPtr<Tool> memory_storage_tool);
  ~ConversationToolProvider() override;

  ConversationToolProvider(const ConversationToolProvider&) = delete;
  ConversationToolProvider& operator=(const ConversationToolProvider&) = delete;

  // ToolProvider implementation
  void OnNewGenerationLoop() override;
  std::vector<base::WeakPtr<Tool>> GetTools() override;

 private:
  std::unique_ptr<TodoTool> todo_tool_ = nullptr;

  // Owned by AIChatService and shared across conversations. It could be
  // invalidated when memory preference is disabled, but it won't leave
  // conversation hanging waiting for a response even if it is destroyed
  // mid-loop because it doesn't have any async operations and will send a
  // response right away in UseTool.
  base::WeakPtr<Tool> memory_storage_tool_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_TOOLS_H_
