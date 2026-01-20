// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api_v2.h"

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

EngineConsumerConversationAPIV2::EngineConsumerConversationAPIV2(
    const mojom::LeoModelOptions& model_options,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager,
    ModelService* model_service,
    PrefService* pref_service)
    : EngineConsumer(model_service, pref_service) {
  DCHECK(!model_options.name.empty());
  model_name_ = model_options.name;
  api_ = std::make_unique<ConversationAPIV2Client>(
      model_options.name, url_loader_factory, credential_manager,
      model_service);
  max_associated_content_length_ = model_options.max_associated_content_length;
}

EngineConsumerConversationAPIV2::~EngineConsumerConversationAPIV2() = default;

void EngineConsumerConversationAPIV2::GenerateQuestionSuggestions(
    PageContents page_contents,
    SuggestedQuestionsCallback callback) {
  auto messages = BuildOAIQuestionSuggestionsMessages(
      page_contents, max_associated_content_length_,
      [this](std::string& input) { SanitizeInput(input); });
  auto on_response = base::BindOnce(
      &EngineConsumerConversationAPIV2::OnGenerateQuestionSuggestionsResponse,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_->PerformRequest(std::move(messages), std::nullopt, std::nullopt,
                       mojom::ConversationCapability::CHAT,
                       base::NullCallback(), std::move(on_response));
}

void EngineConsumerConversationAPIV2::OnGenerateQuestionSuggestionsResponse(
    SuggestedQuestionsCallback callback,
    GenerationResult result) {
  if (!result.has_value()) {
    // Query resulted in error
    std::move(callback).Run(base::unexpected(std::move(result.error())));
    return;
  }

  if (!result->event || !result->event->is_completion_event() ||
      result->event->get_completion_event()->completion.empty()) {
    // No questions were generated
    std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  // Success
  std::vector<std::string> questions =
      base::SplitString(result->event->get_completion_event()->completion, "|",
                        base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  std::move(callback).Run(std::move(questions));
}

void EngineConsumerConversationAPIV2::GenerateAssistantResponse(
    PageContentsMap&& page_contents,
    const ConversationHistory& conversation_history,
    bool is_temporary_chat,
    const std::vector<base::WeakPtr<Tool>>& tools,
    std::optional<std::string_view> preferred_tool_name,
    mojom::ConversationCapability conversation_capability,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  if (!CanPerformCompletionRequest(conversation_history)) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  auto messages =
      BuildOAIMessages(std::move(page_contents), conversation_history, prefs_,
                       is_temporary_chat, max_associated_content_length_,
                       [this](std::string& input) { SanitizeInput(input); });

  // Override model_name to be used if model_key existed for last human turn,
  // used when regenerating answer.
  std::optional<std::string> model_name = std::nullopt;
  const auto& last_entry = conversation_history.back();
  if (last_entry->character_type == mojom::CharacterType::HUMAN &&
      last_entry->model_key) {
    model_name = model_service_->GetLeoModelNameByKey(*last_entry->model_key);
  }

  api_->PerformRequest(std::move(messages), ToolApiDefinitionsFromTools(tools),
                       std::nullopt, conversation_capability,
                       std::move(data_received_callback),
                       std::move(completed_callback), model_name);
}

void EngineConsumerConversationAPIV2::GenerateRewriteSuggestion(
    const std::string& text,
    mojom::ActionType action_type,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  auto messages = BuildOAIRewriteSuggestionMessages(text, action_type);
  if (!messages) {
    std::move(completed_callback)
        .Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }
  api_->PerformRequest(std::move(*messages), std::nullopt, std::nullopt,
                       mojom::ConversationCapability::CHAT,
                       std::move(received_callback),
                       std::move(completed_callback));
}

void EngineConsumerConversationAPIV2::ClearAllQueries() {
  api_->ClearAllQueries();
}

bool EngineConsumerConversationAPIV2::SupportsDeltaTextResponses() const {
  return true;
}

bool EngineConsumerConversationAPIV2::RequiresClientSideTitleGeneration()
    const {
  return true;
}

void EngineConsumerConversationAPIV2::GenerateConversationTitle(
    const PageContentsMap& page_contents,
    const ConversationHistory& conversation_history,
    GenerationCompletedCallback completed_callback) {
  auto messages = BuildOAIGenerateConversationTitleMessages(
      page_contents, conversation_history, max_associated_content_length_,
      [this](std::string& input) { SanitizeInput(input); });

  if (!messages) {
    std::move(completed_callback)
        .Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  api_->PerformRequest(
      std::move(*messages), std::nullopt, std::nullopt,
      mojom::ConversationCapability::CHAT,
      base::NullCallback(),  // no streaming needed
      base::BindOnce(
          &EngineConsumerConversationAPIV2::OnConversationTitleGenerated,
          weak_ptr_factory_.GetWeakPtr(), std::move(completed_callback)));
}

void EngineConsumerConversationAPIV2::DedupeTopics(
    base::expected<std::vector<std::string>, mojom::APIError> topics_result,
    GetSuggestedTopicsCallback callback) {
  if (!topics_result.has_value() || topics_result->empty()) {
    std::move(callback).Run(topics_result);
    return;
  }

  auto messages = BuildOAIDedupeTopicsMessages(*topics_result);

  api_->PerformRequest(
      std::move(messages), std::nullopt, std::nullopt,
      mojom::ConversationCapability::CHAT,
      base::NullCallback() /* data_received_callback */,
      base::BindOnce(
          [](GetSuggestedTopicsCallback callback,
             EngineConsumer::GenerationResult result) {
            // Return deduped topics from the response.
            std::vector<EngineConsumer::GenerationResult> results;
            results.emplace_back(std::move(result));
            std::move(callback).Run(
                EngineConsumer::GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)));
}

void EngineConsumerConversationAPIV2::MergeSuggestTopicsResults(
    GetSuggestedTopicsCallback callback,
    std::vector<GenerationResult> results) {
  if (results.size() == 1) {
    // No need to dedupe topics if there is only one result.
    std::move(callback).Run(
        EngineConsumer::GetStrArrFromTabOrganizationResponses(results));
    return;
  }

  // Merge the result and send another request to dedupe topics.
  DedupeTopics(GetStrArrFromTabOrganizationResponses(results),
               std::move(callback));
}

void EngineConsumerConversationAPIV2::GetSuggestedTopics(
    const std::vector<Tab>& tabs,
    GetSuggestedTopicsCallback callback) {
  auto chunked_messages = BuildChunkedTabFocusMessages(tabs, "");
  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      chunked_messages.size(),
      base::BindOnce(
          &EngineConsumerConversationAPIV2::MergeSuggestTopicsResults,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));

  for (auto& messages : chunked_messages) {
    api_->PerformRequest(std::move(messages), std::nullopt, std::nullopt,
                         mojom::ConversationCapability::CHAT,
                         base::NullCallback() /* data_received_callback */,
                         barrier_callback /* data_completed_callback */);
  }
}

void EngineConsumerConversationAPIV2::GetFocusTabs(
    const std::vector<Tab>& tabs,
    const std::string& topic,
    EngineConsumer::GetFocusTabsCallback callback) {
  auto chunked_messages = BuildChunkedTabFocusMessages(tabs, topic);
  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      chunked_messages.size(),
      base::BindOnce(
          [](EngineConsumer::GetFocusTabsCallback callback,
             std::vector<GenerationResult> results) {
            // Merge the results and call callback with tab IDs or error.
            std::move(callback).Run(
                EngineConsumer::GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)));

  for (auto& messages : chunked_messages) {
    api_->PerformRequest(std::move(messages), std::nullopt, std::nullopt,
                         mojom::ConversationCapability::CHAT,
                         base::NullCallback() /* data_received_callback */,
                         barrier_callback /* data_completed_callback */);
  }
}

}  // namespace ai_chat
