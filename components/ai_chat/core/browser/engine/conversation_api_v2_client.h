// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_V2_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_V2_CLIENT_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class AIChatCredentialManager;
class ModelService;
struct OAIMessage;

// Performs remote request to the remote HTTP Brave Conversation API.
class ConversationAPIV2Client {
 public:
  using GenerationResult = EngineConsumer::GenerationResult;
  using GenerationResultData = EngineConsumer::GenerationResultData;
  using GenerationDataCallback = EngineConsumer::GenerationDataCallback;
  using GenerationCompletedCallback =
      EngineConsumer::GenerationCompletedCallback;

  ConversationAPIV2Client(
      const std::string& model_name,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager,
      ModelService* model_service);
  ConversationAPIV2Client(const ConversationAPIV2Client&) = delete;
  ConversationAPIV2Client& operator=(const ConversationAPIV2Client&) = delete;
  virtual ~ConversationAPIV2Client();

  virtual void PerformRequest(
      std::vector<OAIMessage> messages,
      const std::string& selected_language,
      std::optional<base::Value::List> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      mojom::ConversationCapability conversation_capability,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      const std::optional<std::string>& model_name = std::nullopt);

  void ClearAllQueries();

 protected:
  void SetAPIRequestHelperForTesting(
      std::unique_ptr<api_request_helper::APIRequestHelper> api_helper) {
    api_request_helper_ = std::move(api_helper);
  }
  api_request_helper::APIRequestHelper* GetAPIRequestHelperForTesting() {
    return api_request_helper_.get();
  }

  std::string CreateJSONRequestBody(
      std::vector<OAIMessage> messages,
      const std::string& selected_language,
      std::optional<base::Value::List> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      mojom::ConversationCapability conversation_capability,
      const std::optional<std::string>& model_name,
      const bool is_sse_enabled);

  static base::Value::List SerializeOAIMessages(
      std::vector<OAIMessage> messages);

 private:
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest_ContentBlocks,
                           SerializeOAIMessages_ContentBlocks);

  void PerformRequestWithCredentials(
      std::vector<OAIMessage> messages,
      const std::string& selected_language,
      std::optional<base::Value::List> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      mojom::ConversationCapability conversation_capability,
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
  raw_ptr<AIChatCredentialManager> credential_manager_;
  raw_ptr<ModelService> model_service_;

  base::WeakPtrFactory<ConversationAPIV2Client> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_V2_CLIENT_H_
