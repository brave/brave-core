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

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"
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

// An AI Chat engine consumer that uses the remote HTTP Brave Conversation API.
// Converts between AI Chat's Conversation actions and data model
// (history, associated content, suggested questions, etc.) and the Conversation
// API's request/response format.
class EngineConsumerConversationAPI : public EngineConsumer {
 public:
  explicit EngineConsumerConversationAPI(
      const mojom::LeoModelOptions& model_options,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager,
      ModelService* model_service,
      PrefService* pref_service);
  EngineConsumerConversationAPI(const EngineConsumerConversationAPI&) = delete;
  EngineConsumerConversationAPI& operator=(
      const EngineConsumerConversationAPI&) = delete;
  ~EngineConsumerConversationAPI() override;

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

  // Given a list of tabs, get the suggested topics from the server.
  void GetSuggestedTopics(const std::vector<Tab>& tabs,
                          GetSuggestedTopicsCallback callback) override;
  // Given a list of tabs and a topic, get the focus tabs from the server.
  void GetFocusTabs(const std::vector<Tab>& tabs,
                    const std::string& topic,
                    GetFocusTabsCallback callback) override;

  // A helper function to extract vector of strings from tab organization
  // related responses (e.g. GetSuggestedTopics and GetFocusTabs).
  static base::expected<std::vector<std::string>, mojom::APIError>
  GetStrArrFromTabOrganizationResponses(
      std::vector<EngineConsumer::GenerationResult>& results);

 private:
  // Processes the tab chunks and sends the merge callback with the results.
  // Used by GetSuggestedTopics and GetFocusTabs where the tabs are split into
  // chunks and processed.
  void ProcessTabChunks(
      const std::vector<Tab>& tabs,
      ConversationAPIClient::ConversationEventType event_type,
      base::OnceCallback<void(std::vector<GenerationResult>)> merge_callback,
      const std::string& topic);

  // Merges the result from multiple GetSuggestedTopics requests, which would
  // then trigger DedupeTopics below to get the final topic list with an emoji
  // appended to each topic.
  void MergeSuggestTopicsResults(GetSuggestedTopicsCallback callback,
                                 std::vector<GenerationResult> results);

  // Given a list of results from GetSuggestedTopics, send another request to
  // the server to dedupe the topics.
  void DedupeTopics(
      base::expected<std::vector<std::string>, mojom::APIError> topics_result,
      GetSuggestedTopicsCallback callback);

  void OnGenerateQuestionSuggestionsResponse(
      SuggestedQuestionsCallback callback,
      GenerationResult result);

  ConversationAPIClient::ConversationEvent
  GetAssociatedContentConversationEvent(const PageContent& content,
                                        uint32_t remaining_length);
  std::optional<ConversationAPIClient::ConversationEvent> GetUserMemoryEvent(
      bool is_temporary_chat) const;

  std::unique_ptr<ConversationAPIClient> api_ = nullptr;

  base::WeakPtrFactory<EngineConsumerConversationAPI> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_CONVERSATION_API_H_
