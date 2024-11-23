// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"

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
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/re2/src/re2/re2.h"

namespace ai_chat {

namespace {

using ConversationEvent = ConversationAPIClient::ConversationEvent;
using ConversationEventType = ConversationAPIClient::ConversationEventType;

constexpr size_t kChunkSize = 75;
constexpr char kStrArrPattern[] = R"((\[.+?\]))";

std::vector<std::string> GetStrArrFromResponses(
    std::vector<EngineConsumer::GenerationResult>& results) {
  LOG(ERROR) << "GetStrArrFromResponses" << results.size();
  std::vector<std::string> str_arr;
  for (auto& result : results) {
    if (!result.has_value() || result->empty()) {
      LOG(ERROR) << "no result";
      continue;
    }

    // Remove newline characters from the result.
    base::ReplaceChars(*result, "\n", "", &result.value());
    LOG(ERROR) << result.value();
    std::string strArr = "";
    if (!RE2::PartialMatch(*result, kStrArrPattern, &strArr)) {
      LOG(ERROR) << "Failed to find strArr";
      continue;
    }
    auto value = base::JSONReader::Read(strArr, base::JSON_PARSE_RFC);
    if (!value) {
      LOG(ERROR) << "Failed to parse JSON READ";
      continue;
    }

    auto* list = value->GetIfList();
    if (!list) {
      LOG(ERROR) << "Failed to parse JSON NO LIST";
      continue;
    }

    for (const auto& item : *list) {
      auto* str = item.GetIfString();
      if (!str || str->empty()) {
        LOG(ERROR) << "Failed to parse JSON string";
        continue;
      }
      str_arr.push_back(*str);
    }
  }

  return str_arr;
}

void MergeGetFocusTabsResults(
    EngineConsumer::GetFocusTabsCallback callback,
    std::vector<EngineConsumer::GenerationResult> results) {
  // Merge the results and call callback with tab IDs.
  if (!results.empty()) {
    if (results[0].has_value()) {
      LOG(ERROR) << *results[0];
    }
  }
  std::vector<std::string> tab_ids = GetStrArrFromResponses(results);
  std::move(callback).Run(tab_ids);
}

void OnDedupeTopicsResponse(EngineConsumer::GetSuggestedTopicsCallback callback,
                            EngineConsumer::GenerationResult result) {
  if (!result.has_value() || result->empty()) {
    // std::move(callback).Run(base::unexpected(std::move(result.error())));
    std::move(callback).Run({});
    return;
  }

  std::vector<EngineConsumer::GenerationResult> results = {result};
  std::vector<std::string> topics = GetStrArrFromResponses(results);
  std::move(callback).Run(topics);
}

}  // namespace

EngineConsumerConversationAPI::EngineConsumerConversationAPI(
    const mojom::LeoModelOptions& model_options,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager) {
  DCHECK(!model_options.name.empty());
  api_ = std::make_unique<ConversationAPIClient>(
      model_options.name, url_loader_factory, credential_manager);
  max_associated_content_length_ = model_options.max_associated_content_length;
}

EngineConsumerConversationAPI::~EngineConsumerConversationAPI() = default;

void EngineConsumerConversationAPI::ClearAllQueries() {
  api_->ClearAllQueries();
}

void EngineConsumerConversationAPI::GenerateRewriteSuggestion(
    std::string text,
    const std::string& question,
    const std::string& selected_language,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  std::vector<ConversationEvent> conversation = {
      {mojom::CharacterType::HUMAN, ConversationEventType::UserText, text},
      {mojom::CharacterType::HUMAN, ConversationEventType::RequestRewrite,
       question}};
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
       ConversationEventType::RequestSuggestedActions, ""}};

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
    if (message->selected_text.has_value() &&
        !message->selected_text->empty()) {
      conversation.push_back({mojom::CharacterType::HUMAN,
                              ConversationEventType::PageExcerpt,
                              message->selected_text.value()});
    }
    ConversationEvent event;
    event.role = message->character_type;

    event.content = EngineConsumer::GetPromptForEntry(message);

    // TODO(petemill): Shouldn't the server handle the map of mojom::ActionType
    // to prompts in addition to SUMMARIZE_PAGE (e.g. PARAPHRASE, EXPLAIN,
    // IMPROVE, etc.)
    if (message->action_type == mojom::ActionType::SUMMARIZE_PAGE) {
      event.type = ConversationEventType::RequestSummary;
      event.content = "";
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
  event.content = truncated_page_content;
  // TODO(petemill): Differentiate video transcript / XML / VTT
  event.type = is_video ? ConversationEventType::VideoTranscript
                        : ConversationEventType::PageText;
  return event;
}

void EngineConsumerConversationAPI::GetSuggestedTopics(
    const std::vector<Tab>& tabs,
    GetSuggestedTopicsCallback callback) {
  // Split tabs into chunks of 75 and sent to the server.
  size_t num_chunks = tabs.size() / kChunkSize + 1;
  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      num_chunks,
      base::BindOnce(&EngineConsumerConversationAPI::MergeSuggestTopicsResults,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));

  for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
    base::Value::List tab_value_list;
    for (size_t i = chunk * kChunkSize;
         i < std::min((chunk + 1) * kChunkSize, tabs.size()); ++i) {
      tab_value_list.Append(base::Value::Dict()
                                .Set("id", tabs[i].id)
                                .Set("title", tabs[i].title)
                                .Set("url", tabs[i].url));
    }

    std::vector<ConversationEvent> conversation;
    conversation.push_back(
        {mojom::CharacterType::HUMAN,
         ConversationEventType::GetSuggestedTopicsForFocusTabs,
         base::WriteJson(tab_value_list).value_or(std::string())});

    LOG(ERROR) << "GetSuggestedTopicsForFocusTabs request";
    api_->PerformRequest(std::move(conversation), "" /* selected_language */,
                         base::NullCallback() /* data_received_callback */,
                         barrier_callback /* data_completed_callback */);
  }
}

void EngineConsumerConversationAPI::MergeSuggestTopicsResults(
    GetSuggestedTopicsCallback callback,
    std::vector<GenerationResult> results) {
  // Merge the result and send another request to dedupe topics.
  std::vector<std::string> topics = GetStrArrFromResponses(results);
  DedupeTopics(topics, std::move(callback));
  return;
}

void EngineConsumerConversationAPI::DedupeTopics(
    const std::vector<std::string>& topics,
    GetSuggestedTopicsCallback callback) {
  if (topics.empty()) {
    std::move(callback).Run({});
    return;
  }

  base::Value::List topic_list;
  for (const auto& topic : topics) {
    topic_list.Append(topic);
  }
  std::vector<ConversationEvent> conversation;
  conversation.push_back({mojom::CharacterType::HUMAN,
                          ConversationEventType::DedupeTopics,
                          base::WriteJson(topic_list).value_or(std::string())});
  api_->PerformRequest(
      std::move(conversation), "" /* selected_language */,
      base::NullCallback() /* data_received_callback */,
      base::BindOnce(&OnDedupeTopicsResponse, std::move(callback)));
}

void EngineConsumerConversationAPI::GetFocusTabs(
    const std::vector<Tab>& tabs,
    const std::string& topic,
    EngineConsumer::GetFocusTabsCallback callback) {
  LOG(ERROR) << "GetFocusTabs";
  // Split tab into chunks of 75
  size_t num_chunks = tabs.size() / kChunkSize + 1;
  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      num_chunks,
      base::BindOnce(&MergeGetFocusTabsResults, std::move(callback)));

  for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
    base::Value::List tab_value_list;
    for (size_t i = chunk * kChunkSize;
         i < std::min((chunk + 1) * kChunkSize, tabs.size()); ++i) {
      tab_value_list.Append(base::Value::Dict()
                                .Set("id", tabs[i].id)
                                .Set("title", tabs[i].title)
                                .Set("url", tabs[i].url));
    }

    std::vector<ConversationEvent> conversation;
    conversation.push_back(
        {mojom::CharacterType::HUMAN,
         ConversationEventType::GetFocusTabsForTopic,
         base::WriteJson(tab_value_list).value_or(std::string()), topic});
    LOG(ERROR) << conversation[0].content;
    LOG(ERROR) << conversation[0].type;
    LOG(ERROR) << conversation[0].topic;

    api_->PerformRequest(std::move(conversation), "" /* selected_language */,
                         base::NullCallback() /* data_received_callback */,
                         barrier_callback /* data_completed_callback */);
  }
}

}  // namespace ai_chat
