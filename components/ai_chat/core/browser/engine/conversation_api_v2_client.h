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

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/e2ee_processor.h"
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
      std::optional<base::ListValue> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      const ConversationCapabilitySet& conversation_capabilities,
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
  void SetE2EEProcessorFactoryForTesting(
      base::OnceCallback<std::unique_ptr<E2EEProcessor>()> factory) {
    e2ee_processor_factory_for_testing_ = std::move(factory);
  }

  std::string CreateJSONRequestBody(
      std::vector<OAIMessage> messages,
      std::optional<base::ListValue> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      const ConversationCapabilitySet& conversation_capabilities,
      const std::optional<std::string>& model_name,
      const bool is_sse_enabled,
      const std::optional<std::string>& client_public_key_hex = std::nullopt);

  static base::ListValue SerializeOAIMessages(
      std::vector<OAIMessage> messages,
      const E2EEProcessor::EncryptCallback& encrypt_callback = {});

 private:
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest_ContentBlocks,
                           SerializeOAIMessages_ContentBlocks);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest,
                           OnQueryDataReceived_ContentReceipt);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest,
                           OnQueryDataReceived_CompletionChunk);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest,
                           OnQueryDataReceived_ToolStart);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest,
                           OnQueryDataReceived_InlineSearch);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest,
                           OnQueryDataReceived_ToolCallRequest);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIV2ClientUnitTest,
                           OnQueryDataReceived_ToolCallResult);

  void PerformRequestWithCredentials(
      std::vector<OAIMessage> messages,
      std::optional<base::ListValue> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      const ConversationCapabilitySet& conversation_capabilities,
      const std::optional<std::string>& model_name,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      std::optional<CredentialCacheEntry> credential,
      std::optional<mojom::APIError> attestation_fetch_error);

  void OnQueryCompleted(
      std::optional<CredentialCacheEntry> credential,
      std::optional<E2EEProcessor::ClientSecretKeyBox> secret_key,
      GenerationCompletedCallback callback,
      api_request_helper::APIRequestResult result);

  void OnQueryDataReceived(E2EEProcessor::DecryptCallback decrypt_callback,
                           GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);

  std::optional<std::string> GetLeoModelKeyFromResponse(
      const base::DictValue& response);

  void OnCredentialFetched(
      const std::optional<std::string>& attestation_model_name,
      base::OnceCallback<void(std::optional<CredentialCacheEntry>,
                              std::optional<mojom::APIError>)> callback,
      std::optional<CredentialCacheEntry> credential);

  void EnsureE2EEProcessor();

  const std::string model_name_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  raw_ptr<AIChatCredentialManager> credential_manager_;
  raw_ptr<ModelService> model_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<E2EEProcessor> e2ee_processor_;
  base::OnceCallback<std::unique_ptr<E2EEProcessor>()>
      e2ee_processor_factory_for_testing_;

  base::WeakPtrFactory<ConversationAPIV2Client> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_V2_CLIENT_H_
