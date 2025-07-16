// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/strings/strcat.h"

namespace ai_chat {

EngineConsumer::GenerationResultData::GenerationResultData(
    mojom::ConversationEntryEventPtr event,
    std::optional<std::string>&& model_key)
    : event(std::move(event)), model_key(std::move(model_key)) {}

EngineConsumer::GenerationResultData::GenerationResultData(
    GenerationResultData&& other) = default;
EngineConsumer::GenerationResultData&
EngineConsumer::GenerationResultData::operator=(GenerationResultData&& other) =
    default;

EngineConsumer::GenerationResultData::~GenerationResultData() = default;

// static
std::string EngineConsumer::GetPromptForEntry(
    const mojom::ConversationTurnPtr& entry) {
  const mojom::ConversationTurnPtr& prompt_entry =
      (entry->edits && !entry->edits->empty()) ? entry->edits->back() : entry;

  return prompt_entry->prompt.value_or(prompt_entry->text);
}

EngineConsumer::EngineConsumer(ModelService* model_service)
    : model_service_(model_service) {}
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

  return true;
}

const std::string& EngineConsumer::GetModelName() const {
  return model_name_;
}

}  // namespace ai_chat
