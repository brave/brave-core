// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_OAI_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_OAI_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

template <class T>
class scoped_refptr;

class PrefService;

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
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      ModelService* model_service,
      PrefService* prefs);
  EngineConsumerOAIRemote(const EngineConsumerOAIRemote&) = delete;
  EngineConsumerOAIRemote& operator=(const EngineConsumerOAIRemote&) = delete;
  ~EngineConsumerOAIRemote() override;

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
  void GenerateConversationTitle(
      const PageContentsMap& page_contents,
      const ConversationHistory& conversation_history,
      GenerationCompletedCallback completed_callback) override;
  void SanitizeInput(std::string& input) override;
  void ClearAllQueries() override;
  bool SupportsDeltaTextResponses() const override;
  bool RequiresClientSideTitleGeneration() const override;
  void GetSuggestedTopics(const std::vector<Tab>& tabs,
                          GetSuggestedTopicsCallback callback) override;
  void GetFocusTabs(const std::vector<Tab>& tabs,
                    const std::string& topic,
                    GetFocusTabsCallback callback) override;

  void SetAPIForTesting(std::unique_ptr<OAIAPIClient> api_for_testing) {
    api_ = std::move(api_for_testing);
  }
  OAIAPIClient* GetAPIForTesting() { return api_.get(); }
  void UpdateModelOptions(const mojom::ModelOptions& options) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest, BuildPageContentMessages);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildPageContentMessages_Truncates);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildPageContentMessages_MaxPerContentLength);
  FRIEND_TEST_ALL_PREFIXES(
      EngineConsumerOAIUnitTest,
      BuildPageContentMessages_MaxPerContentLength_UsesRemaining);
  FRIEND_TEST_ALL_PREFIXES(
      EngineConsumerOAIUnitTest,
      BuildPageContentMessages_MaxPerContentLength_NoTruncationNeeded);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildMessages_PageContentsOrderedBeforeTurns);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildMessages_PageContentsExcludedForMissingTurns);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildMessages_MultiplePageContentsForSameTurn);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildMessages_MultiplePageContents_MultipleTurns);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildMessages_EmptyPageContentsMap);
  FRIEND_TEST_ALL_PREFIXES(EngineConsumerOAIUnitTest,
                           BuildMessages_NonExistentTurnId);
  FRIEND_TEST_ALL_PREFIXES(
      EngineConsumerOAIUnitTest,
      BuildMessages_MultiplePageContents_MultipleTurns_TooLong);

  base::Value::List BuildPageContentMessages(
      const PageContents& page_contents,
      uint32_t& max_associated_content_length,
      int video_message_id,
      int page_message_id,
      std::optional<uint32_t> max_per_content_length = std::nullopt);

  base::Value::List BuildMessages(
      const mojom::CustomModelOptions& model_options,
      PageContentsMap& page_contents,
      std::optional<base::Value::Dict> user_memory_message,
      const std::optional<std::string>& selected_text,
      const EngineConsumer::ConversationHistory& conversation_history);

  std::optional<base::Value::Dict> BuildUserMemoryMessage(
      bool is_temporary_chat);

  void OnGenerateQuestionSuggestionsResponse(
      SuggestedQuestionsCallback callback,
      GenerationResult result);

  void OnConversationTitleGenerated(
      GenerationCompletedCallback completion_callback,
      GenerationResult api_result);

  std::unique_ptr<OAIAPIClient> api_ = nullptr;
  mojom::CustomModelOptions model_options_;

  base::WeakPtrFactory<EngineConsumerOAIRemote> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_OAI_H_
