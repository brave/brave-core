// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace base {
class Value;
}  // namespace base

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {
class AIChatCredentialManager;
struct CredentialCacheEntry;

// Performs remote request to the remote HTTP Brave Conversation API.
class ConversationAPIClient {
 public:
  using GenerationResult = EngineConsumer::GenerationResult;
  using GenerationResultData = EngineConsumer::GenerationResultData;
  using GenerationDataCallback = EngineConsumer::GenerationDataCallback;
  using GenerationCompletedCallback =
      EngineConsumer::GenerationCompletedCallback;

  enum ConversationEventType {
    System,
    ContextURL,
    UserText,
    PageText,
    PageExcerpt,
    VideoTranscript,
    VideoTranscriptXML,
    VideoTranscriptVTT,
    BraveSearchResults,
    ChatMessage,
    RequestSuggestedActions,
    RequestSummary,
    RequestRewrite,
    SuggestedActions,
    UploadImage,
    GetSuggestedTopicsForFocusTabs,
    DedupeTopics,
    GetSuggestedAndDedupeTopicsForFocusTabs,
    GetFocusTabsForTopic,
    PageScreenshot,
    // TODO(petemill):
    // - Search in-progress?
    // - Sources?
    // - Entities?
    // - Shouldn't the server handle the map of ai_chat.mojom -> ActionType
    //   to prompts? (e.g. SUMMARIZE_PAGE, PARAPHRASE, etc.)
  };

  struct ConversationEvent {
    mojom::CharacterType role;
    ConversationEventType type;
    std::vector<std::string> content;
    std::string topic;  // Used in GetFocusTabsForTopic event.

    ConversationEvent(mojom::CharacterType,
                      ConversationEventType,
                      const std::vector<std::string>&,
                      const std::string& = "");
    ConversationEvent();
    ~ConversationEvent();
    ConversationEvent(const ConversationEvent&) = delete;
    ConversationEvent& operator=(const ConversationEvent&) = delete;
    ConversationEvent(ConversationEvent&&);
    ConversationEvent& operator=(ConversationEvent&&);
  };

  ConversationAPIClient(
      const std::string& model_name,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager,
      ModelService* model_service);

  ConversationAPIClient(const ConversationAPIClient&) = delete;
  ConversationAPIClient& operator=(const ConversationAPIClient&) = delete;
  virtual ~ConversationAPIClient();

  virtual void PerformRequest(
      std::vector<ConversationEvent> conversation,
      const std::string& selected_language,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      const std::optional<std::string>& model_name = std::nullopt);

  void ClearAllQueries();

  static std::optional<GenerationResultData> ParseResponseEvent(
      base::Value::Dict& response_event,
      ModelService* model_service);

 protected:
  std::string CreateJSONRequestBody(
      const std::vector<ConversationEvent>& conversation,
      const std::string& selected_language,
      const std::optional<std::string>& model_name,
      const bool is_sse_enabled);

  void SetAPIRequestHelperForTesting(
      std::unique_ptr<api_request_helper::APIRequestHelper> api_helper) {
    api_request_helper_ = std::move(api_helper);
  }
  api_request_helper::APIRequestHelper* GetAPIRequestHelperForTesting() {
    return api_request_helper_.get();
  }

 private:
  void PerformRequestWithCredentials(
      std::vector<ConversationEvent> conversation,
      const std::string& selected_language,
      const std::optional<std::string>& model_name,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      std::optional<CredentialCacheEntry> credential);

  void OnQueryCompleted(std::optional<CredentialCacheEntry> credential,
                        GenerationCompletedCallback callback,
                        api_request_helper::APIRequestResult result);
  void OnQueryDataReceived(GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);

  const std::string model_name_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  raw_ptr<AIChatCredentialManager, DanglingUntriaged> credential_manager_;
  raw_ptr<ModelService> model_service_;

  base::WeakPtrFactory<ConversationAPIClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_
