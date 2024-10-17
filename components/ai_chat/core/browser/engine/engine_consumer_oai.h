// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_OAI_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_OAI_H_

#include <memory>
#include <string>
#include <utility>

#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

using api_request_helper::APIRequestResult;

class EngineConsumerOAIRemote : public EngineConsumer {
 public:
  explicit EngineConsumerOAIRemote(
      const mojom::CustomModelOptions& model_options,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  EngineConsumerOAIRemote(const EngineConsumerOAIRemote&) = delete;
  EngineConsumerOAIRemote& operator=(const EngineConsumerOAIRemote&) = delete;
  ~EngineConsumerOAIRemote() override;

  // EngineConsumer
  void GenerateQuestionSuggestions(
      const bool& is_video,
      const std::string& page_content,
      SuggestedQuestionsCallback callback) override;
  void GenerateAssistantResponse(
      const bool& is_video,
      const std::string& page_content,
      const ConversationHistory& conversation_history,
      const std::string& human_input,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback) override;
  void GenerateRewriteSuggestion(
      std::string text,
      const std::string& question,
      GenerationDataCallback received_callback,
      GenerationCompletedCallback completed_callback) override;
  void SanitizeInput(std::string& input) override;
  void ClearAllQueries() override;
  bool SupportsDeltaTextResponses() const override;

  void SetAPIForTesting(std::unique_ptr<OAIAPIClient> api_for_testing) {
    api_ = std::move(api_for_testing);
  }
  OAIAPIClient* GetAPIForTesting() { return api_.get(); }
  void UpdateModelOptions(const mojom::ModelOptions& options) override;

 private:
  void OnGenerateQuestionSuggestionsResponse(
      SuggestedQuestionsCallback callback,
      GenerationResult result);

  std::unique_ptr<OAIAPIClient> api_ = nullptr;
  mojom::CustomModelOptions model_options_;

  base::WeakPtrFactory<EngineConsumerOAIRemote> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_OAI_H_
