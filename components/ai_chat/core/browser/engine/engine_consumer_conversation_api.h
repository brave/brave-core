// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_CONVERSATION_API_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_CONVERSATION_API_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

template <class T>
class scoped_refptr;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class AIChatCredentialManager;
namespace mojom {
class LeoModelOptions;
class ModelOptions;
}  // namespace mojom

// An AI Chat engine consumer that uses the remote HTTP Brave Conversation API.
// Converts between AI Chat's Conversation actions and data model
// (history, associated content, suggested questions, etc.) and the Conversation
// API's request/response format.
class EngineConsumerConversationAPI : public EngineConsumer {
 public:
  explicit EngineConsumerConversationAPI(
      const mojom::LeoModelOptions& model_options,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager);
  EngineConsumerConversationAPI(const EngineConsumerConversationAPI&) = delete;
  EngineConsumerConversationAPI& operator=(
      const EngineConsumerConversationAPI&) = delete;
  ~EngineConsumerConversationAPI() override;

  // EngineConsumer
  void GenerateQuestionSuggestions(
      const bool& is_video,
      const std::string& page_content,
      const std::string& selected_language,
      SuggestedQuestionsCallback callback) override;
  void GenerateAssistantResponse(
      const bool& is_video,
      const std::string& page_content,
      const ConversationHistory& conversation_history,
      const std::string& human_input,
      const std::string& selected_language,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback) override;
  void GenerateRewriteSuggestion(
      std::string text,
      const std::string& question,
      const std::string& selected_language,
      GenerationDataCallback received_callback,
      GenerationCompletedCallback completed_callback) override;
  void SanitizeInput(std::string& input) override;
  void ClearAllQueries() override;
  bool SupportsDeltaTextResponses() const override;

  void SetAPIForTesting(
      std::unique_ptr<ConversationAPIClient> api_for_testing) {
    api_ = std::move(api_for_testing);
  }
  ConversationAPIClient* GetAPIForTesting() { return api_.get(); }
  void UpdateModelOptions(const mojom::ModelOptions& options) override {}

 private:
  void OnGenerateQuestionSuggestionsResponse(
      SuggestedQuestionsCallback callback,
      GenerationResult result);

  ConversationAPIClient::ConversationEvent
  GetAssociatedContentConversationEvent(const std::string& content,
                                        const bool is_video);

  std::unique_ptr<ConversationAPIClient> api_ = nullptr;
  base::WeakPtrFactory<EngineConsumerConversationAPI> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_CONVERSATION_API_H_
