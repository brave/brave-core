// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_CONVERSATION_API_V2_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_CONVERSATION_API_V2_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

template <class T>
class scoped_refptr;

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class AIChatCredentialManager;

namespace mojom {
class LeoModelOptions;
class ModelOptions;
}  // namespace mojom

// An AI Chat engine consumer that uses the remote HTTP Brave Conversation API
// which is using OpenAI API format with some customization, such as a
// customized content block type handled by Brave aichat server.
// Converts between AI Chat's Conversation actions and data model
// (history, associated content, suggested questions, etc.) and the Conversation
// API's request/response format.
class EngineConsumerConversationAPIV2 : public EngineConsumer {
 public:
  EngineConsumerConversationAPIV2(
      const mojom::LeoModelOptions& model_options,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager,
      ModelService* model_service,
      PrefService* pref_service);
  EngineConsumerConversationAPIV2(const EngineConsumerConversationAPIV2&) =
      delete;
  EngineConsumerConversationAPIV2& operator=(
      const EngineConsumerConversationAPIV2&) = delete;
  ~EngineConsumerConversationAPIV2() override;

  // EngineConsumer
  void GenerateQuestionSuggestions(
      PageContents page_contents,
      const std::string& selected_language,
      SuggestedQuestionsCallback callback) override;
  void GenerateAssistantResponse(
      PageContentsMap&& page_contents,
      const ConversationHistory& conversation_history,
      const std::string& selected_language,
      bool is_temporary_chat,
      const std::vector<base::WeakPtr<Tool>>& tools,
      std::optional<std::string_view> preferred_tool_name,
      mojom::ConversationCapability conversation_capability,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback) override;
  void GenerateRewriteSuggestion(
      const std::string& text,
      mojom::ActionType action_type,
      const std::string& selected_language,
      GenerationDataCallback received_callback,
      GenerationCompletedCallback completed_callback) override;
  void SanitizeInput(std::string& input) override {}  // Handle by server.
  void ClearAllQueries() override;
  bool SupportsDeltaTextResponses() const override;
  void UpdateModelOptions(const mojom::ModelOptions& options) override {}
  // Given a list of tabs, get the suggested topics from the server.
  void GetSuggestedTopics(const std::vector<Tab>& tabs,
                          GetSuggestedTopicsCallback callback) override;
  // Given a list of tabs and a topic, get the focus tabs from the server.
  void GetFocusTabs(const std::vector<Tab>& tabs,
                    const std::string& topic,
                    GetFocusTabsCallback callback) override;

  void SetAPIForTesting(
      std::unique_ptr<ConversationAPIV2Client> api_for_testing) {
    api_ = std::move(api_for_testing);
  }
  ConversationAPIV2Client* GetAPIForTesting() { return api_.get(); }

 private:
  std::unique_ptr<ConversationAPIV2Client> api_ = nullptr;

  base::WeakPtrFactory<EngineConsumerConversationAPIV2> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_CONVERSATION_API_V2_H_
