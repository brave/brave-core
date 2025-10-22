// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

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

// static
std::string EngineConsumer::BuildSkillDefinitionMessage(
    const mojom::SkillEntryPtr& skill) {
  return absl::StrFormat("When handling the request, interpret '/%s' as '%s'",
                         skill->shortcut, skill->prompt);
}

EngineConsumer::EngineConsumer(ModelService* model_service, PrefService* prefs)
    : model_service_(model_service), prefs_(prefs) {}
EngineConsumer::~EngineConsumer() = default;

bool EngineConsumer::SupportsDeltaTextResponses() const {
  return false;
}

bool EngineConsumer::RequiresClientSideTitleGeneration() const {
  return false;  // Default: server handles titles (e.g., conversation API)
}

std::string EngineConsumer::GetImageDataURL(base::span<uint8_t> image_data) {
  constexpr char kDataUrlPrefix[] = "data:image/png;base64,";
  return base::StrCat({kDataUrlPrefix, base::Base64Encode(image_data)});
}

std::string EngineConsumer::GetPdfDataURL(base::span<uint8_t> pdf_data) {
  constexpr char kDataUrlPrefix[] = "data:application/pdf;base64,";
  return base::StrCat({kDataUrlPrefix, base::Base64Encode(pdf_data)});
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
