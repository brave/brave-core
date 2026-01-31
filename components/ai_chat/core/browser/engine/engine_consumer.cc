// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/re2/src/re2/re2.h"

namespace ai_chat {

namespace {

constexpr char kArrayPattern[] = R"((?s)(\[.*?\]))";

}  // namespace

EngineConsumer::GenerationResultData::GenerationResultData(
    mojom::ConversationEntryEventPtr event,
    std::optional<std::string> model_key,
    std::optional<bool> is_near_verified)
    : event(std::move(event)),
      model_key(std::move(model_key)),
      is_near_verified(is_near_verified) {}

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

void EngineConsumer::OnConversationTitleGenerated(
    GenerationCompletedCallback completion_callback,
    GenerationResult api_result) {
  // No available title result
  if (!api_result.has_value() || !api_result->event ||
      !api_result->event->is_completion_event() ||
      api_result->event->get_completion_event()->completion.empty()) {
    // Just use the internal error should be fine because currently this error
    // is silently dropped.
    std::move(completion_callback)
        .Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  // Extract and process title from the raw API completion
  std::string_view title = base::TrimWhitespaceASCII(
      api_result->event->get_completion_event()->completion,
      base::TrimPositions::TRIM_ALL);

  // Discard title if longer than our defined max title length
  if (title.length() > kMaxTitleLength) {
    std::move(completion_callback)
        .Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  // Create ConversationTitleEvent
  auto title_event = mojom::ConversationEntryEvent::NewConversationTitleEvent(
      mojom::ConversationTitleEvent::New(std::string(title)));

  GenerationResultData title_result(std::move(title_event), std::nullopt);
  std::move(completion_callback).Run(std::move(title_result));
}

// static
base::expected<std::vector<std::string>, mojom::APIError>
EngineConsumer::GetStrArrFromTabOrganizationResponses(
    std::vector<GenerationResult>& results) {
  // Use RE2 to extract the array from the response, then use rust JSON::Reader
  // to safely parse the array.
  std::vector<std::string> str_arr;
  mojom::APIError error = mojom::APIError::None;
  for (auto& result : results) {
    // Fail the operation if server returns an error, such as rate limiting.
    // On the other hand, ignore the result which cannot be parsed as expected.
    if (!result.has_value()) {
      error = result.error();
      break;
    }

    // Skip empty results.
    if (!result->event || !result->event->is_completion_event() ||
        result->event->get_completion_event()->completion.empty()) {
      continue;
    }

    std::string strArr = "";
    if (!RE2::PartialMatch(result->event->get_completion_event()->completion,
                           kArrayPattern, &strArr)) {
      continue;
    }
    auto value = base::JSONReader::Read(strArr, base::JSON_PARSE_RFC);
    if (!value) {
      continue;
    }

    auto* list = value->GetIfList();
    if (!list) {
      continue;
    }

    for (const auto& item : *list) {
      auto* str = item.GetIfString();
      if (!str || str->empty()) {
        continue;
      }
      str_arr.push_back(*str);
    }
  }

  if (error != mojom::APIError::None) {
    return base::unexpected(error);
  }

  if (str_arr.empty()) {
    return base::unexpected(mojom::APIError::InternalError);
  }

  return str_arr;
}

}  // namespace ai_chat
