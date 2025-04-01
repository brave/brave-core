// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/re2/src/re2/re2.h"

namespace ai_chat {

namespace {

using ConversationEvent = ConversationAPIClient::ConversationEvent;
using ConversationEventType = ConversationAPIClient::ConversationEventType;

constexpr size_t kChunkSize = 75;
constexpr char kArrayPattern[] = R"((\[.*?\]))";

}  // namespace

EngineConsumerConversationAPI::EngineConsumerConversationAPI(
    const mojom::LeoModelOptions& model_options,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager) {
  DCHECK(!model_options.name.empty());
  model_name_ = model_options.name;
  api_ = std::make_unique<ConversationAPIClient>(
      model_options.name, url_loader_factory, credential_manager);
  max_associated_content_length_ = model_options.max_associated_content_length;
}

EngineConsumerConversationAPI::~EngineConsumerConversationAPI() = default;

void EngineConsumerConversationAPI::ClearAllQueries() {
  api_->ClearAllQueries();
}

// static
base::expected<std::vector<std::string>, mojom::APIError>
EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
    std::vector<EngineConsumer::GenerationResult>& results) {
  // Rust implementation of JSON reader is required to parse the response
  // safely. This function currently is only called on Desktop which uses a
  // rust JSON reader. Chromium is in progress of making all platforms use the
  // rust JSON reader and updating the rule of 2 documentation to explicitly
  // point out that JSON parser in base is considered safe.
  if (!base::JSONReader::UsingRust()) {
    return base::unexpected(mojom::APIError::InternalError);
  }

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
    if (result->empty()) {
      continue;
    }

    std::string strArr = "";
    if (!RE2::PartialMatch(*result, kArrayPattern, &strArr)) {
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

void EngineConsumerConversationAPI::GenerateRewriteSuggestion(
    std::string text,
    const std::string& question,
    const std::string& selected_language,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  std::vector<ConversationEvent> conversation = {
      {mojom::CharacterType::HUMAN, ConversationEventType::UserText, {text}},
      {mojom::CharacterType::HUMAN,
       ConversationEventType::RequestRewrite,
       {question}}};

  api_->PerformRequest(std::move(conversation), selected_language,
                       std::move(received_callback),
                       std::move(completed_callback));
}

void EngineConsumerConversationAPI::GenerateQuestionSuggestions(
    const bool& is_video,
    const std::string& page_content,
    const std::string& selected_language,
    SuggestedQuestionsCallback callback) {
  std::vector<ConversationEvent> conversation{
      GetAssociatedContentConversationEvent(page_content, is_video),
      {mojom::CharacterType::HUMAN,
       ConversationEventType::RequestSuggestedActions,
       {""}}};

  auto on_response = base::BindOnce(
      &EngineConsumerConversationAPI::OnGenerateQuestionSuggestionsResponse,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_->PerformRequest(std::move(conversation), selected_language,
                       base::NullCallback(), std::move(on_response));
}

void EngineConsumerConversationAPI::OnGenerateQuestionSuggestionsResponse(
    SuggestedQuestionsCallback callback,
    GenerationResult result) {
  if (!result.has_value() || result->empty()) {
    // Query resulted in error
    std::move(callback).Run(base::unexpected(std::move(result.error())));
    return;
  }

  // Success
  std::vector<std::string> questions =
      base::SplitString(*result, "|", base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  std::move(callback).Run(std::move(questions));
}

void EngineConsumerConversationAPI::GenerateAssistantResponse(
    const bool& is_video,
    const std::string& page_content,
    const ConversationHistory& conversation_history,
    const std::string& selected_language,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  if (!CanPerformCompletionRequest(conversation_history)) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  std::vector<ConversationEvent> conversation;
  // associated content
  if (!page_content.empty()) {
    conversation.push_back(
        GetAssociatedContentConversationEvent(page_content, is_video));
  }
  // history
  for (const auto& message : conversation_history) {
    if (message->uploaded_images) {
      std::vector<std::string> uploaded_images;
      std::vector<std::string> screenshot_images;
      for (const auto& uploaded_image : message->uploaded_images.value()) {
        if (base::StartsWith(uploaded_image->filename, "fullscreenshot")) {
          screenshot_images.emplace_back(
              GetImageDataURL(uploaded_image->image_data));
        } else {
          uploaded_images.emplace_back(
              GetImageDataURL(uploaded_image->image_data));
        }
      }
      if (!uploaded_images.empty()) {
        conversation.push_back({mojom::CharacterType::HUMAN,
                                ConversationEventType::UploadImage,
                                std::move(uploaded_images)});
      }
      if (!screenshot_images.empty()) {
        conversation.push_back({mojom::CharacterType::HUMAN,
                                ConversationEventType::PageScreenshot,
                                std::move(screenshot_images)});
      }
    }
    if (message->selected_text.has_value() &&
        !message->selected_text->empty()) {
      conversation.push_back({mojom::CharacterType::HUMAN,
                              ConversationEventType::PageExcerpt,
                              {message->selected_text.value()}});
    }
    ConversationEvent event;
    event.role = message->character_type;

    event.content = {EngineConsumer::GetPromptForEntry(message)};

    // TODO(petemill): Shouldn't the server handle the map of mojom::ActionType
    // to prompts in addition to SUMMARIZE_PAGE (e.g. PARAPHRASE, EXPLAIN,
    // IMPROVE, etc.)
    if (message->action_type == mojom::ActionType::SUMMARIZE_PAGE) {
      event.type = ConversationEventType::RequestSummary;
      event.content = {""};
    } else {
      event.type = ConversationEventType::ChatMessage;
    }
    conversation.push_back(std::move(event));
  }

  api_->PerformRequest(std::move(conversation), selected_language,
                       std::move(data_received_callback),
                       std::move(completed_callback));
}

void EngineConsumerConversationAPI::SanitizeInput(std::string& input) {
  // Sanitization is handled by the server
}

bool EngineConsumerConversationAPI::SupportsDeltaTextResponses() const {
  return true;
}

ConversationEvent
EngineConsumerConversationAPI::GetAssociatedContentConversationEvent(
    const std::string& content,
    const bool is_video) {
  const std::string& truncated_page_content =
      content.substr(0, max_associated_content_length_);

  ConversationEvent event;
  event.role = mojom::CharacterType::HUMAN;
  event.content = {truncated_page_content};
  // TODO(petemill): Differentiate video transcript / XML / VTT
  event.type = is_video ? ConversationEventType::VideoTranscript
                        : ConversationEventType::PageText;
  return event;
}

void EngineConsumerConversationAPI::DedupeTopics(
    base::expected<std::vector<std::string>, mojom::APIError> topics_result,
    GetSuggestedTopicsCallback callback) {
  if (!topics_result.has_value() || topics_result->empty()) {
    std::move(callback).Run(topics_result);
    return;
  }

  base::Value::List topic_list;
  for (const auto& topic : *topics_result) {
    topic_list.Append(topic);
  }
  std::vector<ConversationEvent> conversation;
  conversation.push_back(
      {mojom::CharacterType::HUMAN,
       ConversationEventType::DedupeTopics,
       {base::WriteJson(topic_list).value_or(std::string())}});
  api_->PerformRequest(
      std::move(conversation), "" /* selected_language */,
      base::NullCallback() /* data_received_callback */,
      base::BindOnce(
          [](GetSuggestedTopicsCallback callback,
             EngineConsumer::GenerationResult result) {
            // Return deduped topics from the response.
            std::vector<EngineConsumer::GenerationResult> results = {result};
            std::move(callback).Run(
                EngineConsumerConversationAPI::
                    GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)));
}

void EngineConsumerConversationAPI::ProcessTabChunks(
    const std::vector<Tab>& tabs,
    ConversationEventType event_type,
    base::OnceCallback<void(std::vector<GenerationResult>)> merge_callback,
    const std::string& topic) {
  CHECK(event_type == ConversationEventType::GetSuggestedTopicsForFocusTabs ||
        event_type ==
            ConversationEventType::GetSuggestedAndDedupeTopicsForFocusTabs ||
        event_type == ConversationEventType::GetFocusTabsForTopic);

  // Split tab into chunks of 75
  size_t num_chunks = (tabs.size() + kChunkSize - 1) / kChunkSize;
  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      num_chunks, std::move(merge_callback));

  for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
    base::Value::List tab_value_list;
    for (size_t i = chunk * kChunkSize;
         i < std::min((chunk + 1) * kChunkSize, tabs.size()); ++i) {
      tab_value_list.Append(base::Value::Dict()
                                .Set("id", tabs[i].id)
                                .Set("title", tabs[i].title)
                                .Set("url", tabs[i].origin.Serialize()));
    }

    std::vector<ConversationEvent> conversation;
    conversation.push_back(
        {mojom::CharacterType::HUMAN,
         event_type,
         {base::WriteJson(tab_value_list).value_or(std::string())},
         topic});

    api_->PerformRequest(std::move(conversation), "" /* selected_language */,
                         base::NullCallback() /* data_received_callback */,
                         barrier_callback /* data_completed_callback */);
  }
}

void EngineConsumerConversationAPI::MergeSuggestTopicsResults(
    GetSuggestedTopicsCallback callback,
    std::vector<GenerationResult> results) {
  if (results.size() == 1) {
    // No need to dedupe topics if there is only one result.
    std::move(callback).Run(
        EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
            results));
    return;
  }

  // Merge the result and send another request to dedupe topics.
  DedupeTopics(GetStrArrFromTabOrganizationResponses(results),
               std::move(callback));
}

void EngineConsumerConversationAPI::GetSuggestedTopics(
    const std::vector<Tab>& tabs,
    GetSuggestedTopicsCallback callback) {
  auto event_type =
      tabs.size() > kChunkSize
          ? ConversationEventType::GetSuggestedTopicsForFocusTabs
          : ConversationEventType::GetSuggestedAndDedupeTopicsForFocusTabs;
  ProcessTabChunks(
      tabs, event_type,
      base::BindOnce(&EngineConsumerConversationAPI::MergeSuggestTopicsResults,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      "" /* topic */);
}

void EngineConsumerConversationAPI::GetFocusTabs(
    const std::vector<Tab>& tabs,
    const std::string& topic,
    EngineConsumer::GetFocusTabsCallback callback) {
  ProcessTabChunks(
      tabs, ConversationEventType::GetFocusTabsForTopic,
      base::BindOnce(
          [](EngineConsumer::GetFocusTabsCallback callback,
             std::vector<GenerationResult> results) {
            // Merge the results and call callback with tab IDs or
            // error.
            std::move(callback).Run(
                EngineConsumerConversationAPI::
                    GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)),
      topic);
}

}  // namespace ai_chat
