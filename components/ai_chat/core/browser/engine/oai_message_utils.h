// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_MESSAGE_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_MESSAGE_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/function_ref.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

class PrefService;

namespace ai_chat {

struct OAIMessage {
  OAIMessage();
  OAIMessage(const OAIMessage&) = delete;
  OAIMessage& operator=(const OAIMessage&) = delete;
  OAIMessage(OAIMessage&&);
  OAIMessage& operator=(OAIMessage&&);
  ~OAIMessage();

  std::string role;
  std::vector<mojom::ContentBlockPtr> content;
};

std::vector<OAIMessage> BuildOAIMessages(
    PageContentsMap&& page_contents,
    const EngineConsumer::ConversationHistory& conversation_history,
    PrefService* prefs,
    bool is_temporary_chat,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input);

std::vector<OAIMessage> BuildOAIQuestionSuggestionsMessages(
    PageContents page_contents,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input);

std::optional<std::vector<OAIMessage>> BuildOAIRewriteSuggestionMessages(
    const std::string& text,
    mojom::ActionType action_type);

std::optional<std::vector<OAIMessage>>
BuildOAIGenerateConversationTitleMessages(
    const PageContentsMap& page_contents,
    const EngineConsumer::ConversationHistory& conversation_history,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input);

OAIMessage BuildOAISeedMessage(const std::string& text);

// Tab organization message building functions

// Build OAI messages for deduping topics.
std::vector<OAIMessage> BuildOAIDedupeTopicsMessages(
    const std::vector<std::string>& topics);

// Given a list of tabs, split them into chunks and build messages for each
// chunk. Topic is non-empty for getting focus tabs (filter tabs based on a
// topic), otherwise it's empty for getting suggested topics.
std::vector<std::vector<OAIMessage>> BuildChunkedTabFocusMessages(
    const std::vector<Tab>& tabs,
    const std::string& topic = "");

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_MESSAGE_UTILS_H_
