// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/devtools/ai_chat_handler.h"

#include <utility>

#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "content/browser/devtools/protocol/browser_handler.h"

namespace brave::devtools {

using content::protocol::DispatchResponse;
using content::protocol::String;

namespace {
AIChatHandler::ServiceGetter g_service_getter = nullptr;
}

// static
void AIChatHandler::SetServiceGetter(ServiceGetter getter) {
  g_service_getter = getter;
}

AIChatHandler::AIChatHandler(content::DevToolsAgentHostImpl* /*agent_host*/)
    : content::protocol::DevToolsDomainHandler(
          content::protocol::BraveAIChat::Metainfo::domainName) {}

AIChatHandler::~AIChatHandler() {
  StopObservingAll();
}

void AIChatHandler::Wire(content::protocol::UberDispatcher* dispatcher) {
  frontend_ = std::make_unique<content::protocol::BraveAIChat::Frontend>(
      dispatcher->channel());
  content::protocol::BraveAIChat::Dispatcher::wire(dispatcher, this);
}

DispatchResponse AIChatHandler::Enable(
    std::optional<String> in_browserContextId) {
  if (enabled_) {
    return DispatchResponse::Success();
  }

  if (!g_service_getter) {
    return DispatchResponse::ServerError(
        "AI Chat service getter not registered");
  }

  content::BrowserContext* browser_context = nullptr;
  DispatchResponse response =
      content::protocol::BrowserHandler::FindBrowserContext(
          in_browserContextId, &browser_context);
  if (!response.IsSuccess()) {
    return response;
  }

  ai_chat_service_ = g_service_getter(browser_context);
  if (!ai_chat_service_) {
    return DispatchResponse::ServerError("AI Chat service is not available");
  }

  enabled_ = true;
  return DispatchResponse::Success();
}

DispatchResponse AIChatHandler::Disable() {
  if (!enabled_) {
    return DispatchResponse::Success();
  }

  StopObservingAll();
  conversations_.clear();
  ai_chat_service_ = nullptr;
  enabled_ = false;
  return DispatchResponse::Success();
}

void AIChatHandler::CreateConversation(
    std::unique_ptr<CreateConversationCallback> callback) {
  if (!enabled_ || !ai_chat_service_) {
    callback->sendFailure(
        DispatchResponse::ServerError("Domain not enabled"));
    return;
  }

  ai_chat::ConversationHandler* handler =
      ai_chat_service_->CreateConversation();
  if (!handler) {
    callback->sendFailure(
        DispatchResponse::ServerError("Failed to create conversation"));
    return;
  }

  std::string uuid = handler->get_conversation_uuid();
  handler->AddObserver(this);
  conversations_[uuid] = handler;

  callback->sendSuccess(uuid);
}

void AIChatHandler::SubmitMessage(
    const String& in_conversationId,
    const String& in_message,
    std::unique_ptr<SubmitMessageCallback> callback) {
  if (!enabled_) {
    callback->sendFailure(
        DispatchResponse::ServerError("Domain not enabled"));
    return;
  }

  auto it = conversations_.find(in_conversationId);
  if (it == conversations_.end()) {
    callback->sendFailure(DispatchResponse::InvalidParams(
        "Unknown conversation: " + in_conversationId));
    return;
  }

  ai_chat::ConversationHandler* handler = it->second;
  handler->SubmitHumanConversationEntry(in_message, std::nullopt);

  // Resolve immediately; the response arrives via turnCompleted event.
  callback->sendSuccess();
}

void AIChatHandler::GetHistory(
    const String& in_conversationId,
    std::unique_ptr<GetHistoryCallback> callback) {
  if (!enabled_) {
    callback->sendFailure(
        DispatchResponse::ServerError("Domain not enabled"));
    return;
  }

  auto it = conversations_.find(in_conversationId);
  if (it == conversations_.end()) {
    callback->sendFailure(DispatchResponse::InvalidParams(
        "Unknown conversation: " + in_conversationId));
    return;
  }

  ai_chat::ConversationHandler* handler = it->second;
  const auto& history = handler->GetConversationHistory();

  auto entries = std::make_unique<content::protocol::Array<
      content::protocol::BraveAIChat::ConversationEntry>>();

  for (const auto& turn : history) {
    auto tool_uses = std::make_unique<content::protocol::Array<
        content::protocol::BraveAIChat::ToolUseInfo>>();

    if (turn->events) {
      for (const auto& event : *turn->events) {
        if (event->is_tool_use_event()) {
          const auto& tool_event = event->get_tool_use_event();
          auto tool_info =
              content::protocol::BraveAIChat::ToolUseInfo::Create()
                  .SetId(tool_event->id)
                  .SetToolName(tool_event->tool_name)
                  .Build();
          tool_uses->push_back(std::move(tool_info));
        }
      }
    }

    std::string character_type =
        turn->character_type == ai_chat::mojom::CharacterType::HUMAN
            ? "human"
            : "assistant";

    auto entry = content::protocol::BraveAIChat::ConversationEntry::Create()
                     .SetUuid(turn->uuid.value_or(""))
                     .SetCharacterType(character_type)
                     .SetText(turn->text)
                     .SetTimestamp(turn->created_time.InSecondsFSinceUnixEpoch())
                     .Build();
    if (!tool_uses->empty()) {
      entry->SetToolUses(std::move(tool_uses));
    }
    entries->push_back(std::move(entry));
  }

  callback->sendSuccess(std::move(entries));
}

DispatchResponse AIChatHandler::DestroyConversation(
    const String& in_conversationId) {
  auto it = conversations_.find(in_conversationId);
  if (it == conversations_.end()) {
    return DispatchResponse::InvalidParams("Unknown conversation: " +
                                           in_conversationId);
  }

  it->second->RemoveObserver(this);
  conversations_.erase(it);
  return DispatchResponse::Success();
}

// ConversationHandler::Observer

void AIChatHandler::OnRequestInProgressChanged(
    ai_chat::ConversationHandler* handler,
    bool in_progress) {
  if (in_progress) {
    return;
  }

  auto conversation_id = FindConversationId(handler);
  if (!conversation_id) {
    return;
  }

  const auto& history = handler->GetConversationHistory();
  if (history.empty()) {
    return;
  }

  const auto& last_entry = history.back();
  if (last_entry->character_type != ai_chat::mojom::CharacterType::ASSISTANT) {
    return;
  }

  auto tool_uses = std::make_unique<content::protocol::Array<
      content::protocol::BraveAIChat::ToolUseInfo>>();

  if (last_entry->events) {
    for (const auto& event : *last_entry->events) {
      if (event->is_tool_use_event()) {
        const auto& tool_event = event->get_tool_use_event();
        auto tool_info =
            content::protocol::BraveAIChat::ToolUseInfo::Create()
                .SetId(tool_event->id)
                .SetToolName(tool_event->tool_name)
                .Build();
        tool_uses->push_back(std::move(tool_info));
      }
    }
  }

  frontend_->TurnCompleted(*conversation_id,
                           last_entry->uuid.value_or(""),
                           last_entry->text, std::move(tool_uses));
}

void AIChatHandler::OnToolUseEventOutput(
    ai_chat::ConversationHandler* handler,
    std::string_view entry_uuid,
    size_t event_order,
    ai_chat::mojom::ToolUseEventPtr tool_use) {
  auto conversation_id = FindConversationId(handler);
  if (!conversation_id) {
    return;
  }

  std::string progress_type =
      tool_use->output ? "toolCompleted" : "toolStarted";

  frontend_->TurnProgress(*conversation_id, progress_type,
                           /* text */ {},
                           /* toolName */ tool_use->tool_name,
                           /* toolStatus */ progress_type);
}

void AIChatHandler::OnConversationTitleChanged(
    const std::string& conversation_uuid,
    const std::string& title) {
  auto it = conversations_.find(conversation_uuid);
  if (it == conversations_.end()) {
    return;
  }

  frontend_->ConversationTitleChanged(conversation_uuid, title);
}

std::optional<std::string> AIChatHandler::FindConversationId(
    ai_chat::ConversationHandler* handler) const {
  for (const auto& [uuid, h] : conversations_) {
    if (h == handler) {
      return uuid;
    }
  }
  return std::nullopt;
}

void AIChatHandler::StopObservingAll() {
  for (auto& [uuid, handler] : conversations_) {
    if (handler) {
      handler->RemoveObserver(this);
    }
  }
}

}  // namespace brave::devtools
