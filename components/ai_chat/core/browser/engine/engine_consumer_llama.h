// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_LLAMA_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_LLAMA_H_

#include <memory>
#include <string>

#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

using api_request_helper::APIRequestResult;

// An AI Chat engine consumer that uses the Claude-style remote HTTP completion
// API and builds prompts tailored to the Brave Leo model.
class EngineConsumerLlamaRemote : public EngineConsumer {
 public:
  explicit EngineConsumerLlamaRemote(
      const mojom::Model& model,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager);
  EngineConsumerLlamaRemote(const EngineConsumerLlamaRemote&) = delete;
  EngineConsumerLlamaRemote& operator=(const EngineConsumerLlamaRemote&) =
      delete;
  ~EngineConsumerLlamaRemote() override;

  // EngineConsumer
  void GenerateQuestionSuggestions(
      const bool& is_video,
      const std::string& page_content,
      SuggestedQuestionsCallback callback) override;
  void GenerateAssistantResponse(
      const bool& is_video,
      const std::string& page_content,
      std::optional<std::string> selected_text,
      const ConversationHistory& conversation_history,
      const std::string& human_input,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback) override;
  void SanitizeInput(std::string& input) override;
  void ClearAllQueries() override;

 private:
  void OnGenerateQuestionSuggestionsResponse(
      SuggestedQuestionsCallback callback,
      GenerationResult result);

  bool needs_general_seed_ = true;

  base::WeakPtrFactory<EngineConsumerLlamaRemote> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_LLAMA_H_
