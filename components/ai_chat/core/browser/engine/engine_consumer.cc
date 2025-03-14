// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/containers/flat_tree.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

// static
std::string EngineConsumer::GetPromptForEntry(
    const mojom::ConversationTurnPtr& entry) {
  const mojom::ConversationTurnPtr& prompt_entry =
      (entry->edits && !entry->edits->empty()) ? entry->edits->back() : entry;

  return prompt_entry->prompt.value_or(prompt_entry->text);
}

EngineConsumer::EngineConsumer() = default;
EngineConsumer::~EngineConsumer() = default;

bool EngineConsumer::SupportsDeltaTextResponses() const {
  return false;
}

std::string EngineConsumer::GetImageDataURL(base::span<uint8_t> image_data) {
  constexpr char kDataUrlPrefix[] = "data:image/png;base64,";
  return base::StrCat({kDataUrlPrefix, base::Base64Encode(image_data)});
}

bool EngineConsumer::CanPerformCompletionRequest(
    const ConversationHistory& conversation_history) const {
  if (conversation_history.empty()) {
    return false;
  }

  const auto& last_turn = conversation_history.back();

  if (last_turn->character_type == mojom::CharacterType::ASSISTANT) {
    // If we don't have a human entry to submit then we might have
    // tool use responses to submit and continue the assistant response with.
    if (last_turn->events.has_value() && !last_turn->events->empty()) {
      if (std::ranges::any_of(
              *last_turn->events,
              [](const mojom::ConversationEntryEventPtr& event) {
                return event->is_tool_use_event() &&
                       event->get_tool_use_event()->output_json.has_value();
              })) {
        return true;
      }
    }
    return false;
  }

  return true;
}

}  // namespace ai_chat
