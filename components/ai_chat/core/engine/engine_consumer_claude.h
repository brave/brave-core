// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_ENGINE_ENGINE_CONSUMER_CLAUDE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_ENGINE_ENGINE_CONSUMER_CLAUDE_H_

#include <memory>
#include <string>

#include "brave/components/ai_chat/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/engine/remote_completion_client.h"

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

using api_request_helper::APIRequestResult;

// An AI Chat engine consumer that uses the Claude-style remote HTTP completion
// API and builds prompts tailored to the Claude models.
class EngineConsumerClaudeRemote : public EngineConsumer {
 public:
  explicit EngineConsumerClaudeRemote(
      const mojom::Model& model,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager);
  EngineConsumerClaudeRemote(const EngineConsumerClaudeRemote&) = delete;
  EngineConsumerClaudeRemote& operator=(const EngineConsumerClaudeRemote&) =
      delete;
  ~EngineConsumerClaudeRemote() override;

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
  void SanitizeInput(std::string& input) override;
  void ClearAllQueries() override;

 private:
  void OnGenerateQuestionSuggestionsResponse(
      SuggestedQuestionsCallback callback,
      GenerationResult result);

  std::unique_ptr<RemoteCompletionClient> api_ = nullptr;

  const mojom::Model& model_;

  base::WeakPtrFactory<EngineConsumerClaudeRemote> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_ENGINE_ENGINE_CONSUMER_CLAUDE_H_
