// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

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

// Performs remote request to the remote HTTP Brave Conversation API.
class ConversationAPIClient {
 public:
  using GenerationResult = base::expected<std::string, mojom::APIError>;
  using GenerationDataCallback =
      base::RepeatingCallback<void(mojom::ConversationEntryEventPtr)>;
  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

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
    std::string content;
  };

  ConversationAPIClient(
      const std::string& model_name,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager);

  ConversationAPIClient(const ConversationAPIClient&) = delete;
  ConversationAPIClient& operator=(const ConversationAPIClient&) = delete;
  virtual ~ConversationAPIClient();

  virtual void PerformRequest(
      const std::vector<ConversationEvent>& conversation,
      const std::string& selected_language,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback);

  void ClearAllQueries();

 protected:
  std::string CreateJSONRequestBody(
      const std::vector<ConversationEvent>& conversation,
      const std::string& selected_language,
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
      const std::vector<ConversationEvent>& conversation,
      const std::string selected_language,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      std::optional<CredentialCacheEntry> credential);

  void OnQueryCompleted(std::optional<CredentialCacheEntry> credential,
                        GenerationCompletedCallback callback,
                        APIRequestResult result);
  void OnQueryDataReceived(GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);

  const std::string model_name_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  raw_ptr<AIChatCredentialManager, DanglingUntriaged> credential_manager_;

  base::WeakPtrFactory<ConversationAPIClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_
