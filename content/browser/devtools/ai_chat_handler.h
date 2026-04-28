// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CONTENT_BROWSER_DEVTOOLS_AI_CHAT_HANDLER_H_
#define BRAVE_CONTENT_BROWSER_DEVTOOLS_AI_CHAT_HANDLER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "content/browser/devtools/protocol/brave_ai_chat.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "content/common/content_export.h"

namespace content {
class BrowserContext;
class DevToolsAgentHostImpl;
}  // namespace content

namespace ai_chat {
class AIChatService;
}  // namespace ai_chat

namespace brave::devtools {

class AIChatHandler : public content::protocol::DevToolsDomainHandler,
                      public content::protocol::BraveAIChat::Backend,
                      public ai_chat::ConversationHandler::Observer {
 public:
  // Function type for looking up AIChatService from a BrowserContext.
  // Set by the browser layer at startup via SetServiceGetter().
  using ServiceGetter =
      ai_chat::AIChatService* (*)(content::BrowserContext*);

  // Must be called before any AIChatHandler is used (typically during
  // browser initialization).
  CONTENT_EXPORT static void SetServiceGetter(ServiceGetter getter);

  explicit AIChatHandler(content::DevToolsAgentHostImpl* agent_host);
  ~AIChatHandler() override;

  AIChatHandler(const AIChatHandler&) = delete;
  AIChatHandler& operator=(const AIChatHandler&) = delete;

  // DevToolsDomainHandler
  void Wire(content::protocol::UberDispatcher* dispatcher) override;

  // BraveAIChat::Backend
  content::protocol::DispatchResponse Enable(
      std::optional<content::protocol::String> in_browserContextId) override;
  content::protocol::DispatchResponse Disable() override;
  void CreateConversation(
      std::unique_ptr<CreateConversationCallback> callback) override;
  void SubmitMessage(
      const content::protocol::String& in_conversationId,
      const content::protocol::String& in_message,
      std::unique_ptr<SubmitMessageCallback> callback) override;
  void GetHistory(
      const content::protocol::String& in_conversationId,
      std::unique_ptr<GetHistoryCallback> callback) override;
  content::protocol::DispatchResponse DestroyConversation(
      const content::protocol::String& in_conversationId) override;

  // ai_chat::ConversationHandler::Observer
  void OnRequestInProgressChanged(ai_chat::ConversationHandler* handler,
                                  bool in_progress) override;
  void OnToolUseEventOutput(ai_chat::ConversationHandler* handler,
                            std::string_view entry_uuid,
                            size_t event_order,
                            ai_chat::mojom::ToolUseEventPtr tool_use) override;
  void OnConversationTitleChanged(const std::string& conversation_uuid,
                                  const std::string& title) override;

 private:
  std::optional<std::string> FindConversationId(
      ai_chat::ConversationHandler* handler) const;

  void StopObservingAll();

  raw_ptr<ai_chat::AIChatService> ai_chat_service_ = nullptr;
  std::unique_ptr<content::protocol::BraveAIChat::Frontend> frontend_;
  bool enabled_ = false;

  std::map<std::string, raw_ptr<ai_chat::ConversationHandler>> conversations_;
};

}  // namespace brave::devtools

#endif  // BRAVE_CONTENT_BROWSER_DEVTOOLS_AI_CHAT_HANDLER_H_
