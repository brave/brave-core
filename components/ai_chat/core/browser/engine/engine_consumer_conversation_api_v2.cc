// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api_v2.h"

#include "base/check.h"
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
    const std::string& selected_language,
    SuggestedQuestionsCallback callback) {
  std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
}

void EngineConsumerConversationAPIV2::GenerateAssistantResponse(
    PageContentsMap&& page_contents,
    const ConversationHistory& conversation_history,
    const std::string& selected_language,
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
      BuildOAIMessages(std::move(page_contents), conversation_history,
                       max_associated_content_length_);

  // Override model_name to be used if model_key existed, used when
  // regenerating answer.
  std::optional<std::string> model_name = std::nullopt;
  if (conversation_history.back()->model_key) {
    model_name = model_service_->GetLeoModelNameByKey(
        *conversation_history.back()->model_key);
  }

  api_->PerformRequest(std::move(messages), selected_language,
                       ToolApiDefinitionsFromTools(tools), std::nullopt,
                       conversation_capability,
                       std::move(data_received_callback),
                       std::move(completed_callback), model_name);
}

void EngineConsumerConversationAPIV2::GenerateRewriteSuggestion(
    const std::string& text,
    mojom::ActionType action_type,
    const std::string& selected_language,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  auto messages = BuildOAIRewriteSuggestionMessages(text, action_type);
  if (!messages) {
    std::move(completed_callback)
        .Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }
  api_->PerformRequest(std::move(*messages), selected_language, std::nullopt,
                       std::nullopt, mojom::ConversationCapability::CHAT,
                       std::move(received_callback),
                       std::move(completed_callback));
}

void EngineConsumerConversationAPIV2::ClearAllQueries() {
  api_->ClearAllQueries();
}

bool EngineConsumerConversationAPIV2::SupportsDeltaTextResponses() const {
  return true;
}

void EngineConsumerConversationAPIV2::GetSuggestedTopics(
    const std::vector<Tab>& tabs,
    GetSuggestedTopicsCallback callback) {
  std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
}

void EngineConsumerConversationAPIV2::GetFocusTabs(
    const std::vector<Tab>& tabs,
    const std::string& topic,
    GetFocusTabsCallback callback) {
  std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
}

}  // namespace ai_chat
