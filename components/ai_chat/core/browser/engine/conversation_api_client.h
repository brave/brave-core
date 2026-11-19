// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_endpoints.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/endpoint_client/with_headers.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class AIChatCredentialManager;
class ModelService;
struct OAIMessage;

// Performs remote request to the remote HTTP Brave Conversation API.
class ConversationAPIClient {
 public:
  using GenerationResult = EngineConsumer::GenerationResult;
  using GenerationResultData = EngineConsumer::GenerationResultData;
  using GenerationDataCallback = EngineConsumer::GenerationDataCallback;
  using GenerationCompletedCallback =
      EngineConsumer::GenerationCompletedCallback;

  ConversationAPIClient(
      const std::string& model_name,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      AIChatCredentialManager* credential_manager,
      ModelService* model_service);
  ConversationAPIClient(const ConversationAPIClient&) = delete;
  ConversationAPIClient& operator=(const ConversationAPIClient&) = delete;
  virtual ~ConversationAPIClient();

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
  base::DictValue CreateRequestBody(
      std::vector<OAIMessage> messages,
      std::optional<base::ListValue> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      const ConversationCapabilitySet& conversation_capabilities,
      const std::optional<std::string>& model_name,
      const bool is_sse_enabled);

  std::string CreateJSONRequestBody(
      std::vector<OAIMessage> messages,
      std::optional<base::ListValue> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      const ConversationCapabilitySet& conversation_capabilities,
      const std::optional<std::string>& model_name,
      const bool is_sse_enabled);

  static base::ListValue SerializeOAIMessages(std::vector<OAIMessage> messages);

 private:
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIClientUnitTest_ContentBlocks,
                           SerializeOAIMessages_ContentBlocks);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIClientUnitTest,
                           OnQueryDataReceived_ContentReceipt);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIClientUnitTest,
                           OnQueryDataReceived_CompletionChunk);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIClientUnitTest,
                           OnQueryDataReceived_ToolStart);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIClientUnitTest,
                           OnQueryDataReceived_InlineSearch);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIClientUnitTest,
                           OnQueryDataReceived_ToolCallRequest);
  FRIEND_TEST_ALL_PREFIXES(ConversationAPIClientUnitTest,
                           OnQueryDataReceived_ToolCallResult);

  void PerformRequestWithCredentials(
      std::vector<OAIMessage> messages,
      std::optional<base::ListValue> oai_tool_definitions,
      const std::optional<std::string>& preferred_tool_name,
      const ConversationCapabilitySet& conversation_capabilities,
      const std::optional<std::string>& model_name,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      std::optional<CredentialCacheEntry> credential);

  // An in-flight request.
  struct Query;
  // The outcome of a request, whether it streamed or not.
  struct QueryResult;

  static QueryResult MakeQueryResult(
      endpoint_client::WithHeaders<ConversationAPIEndpoint::Response> response);
  static QueryResult MakeStreamQueryResult(
      ConversationAPIStreamEndpoint::Response::Result result);

  void OnStreamStarted(uint64_t query_id,
                       scoped_refptr<net::HttpResponseHeaders> headers);
  void OnStreamChunk(uint64_t query_id, std::string_view chunk);
  void OnStreamCompleted(uint64_t query_id,
                         std::optional<CredentialCacheEntry> credential,
                         GenerationCompletedCallback callback,
                         QueryResult result);
  // Forgets the query and completes it, unless it has been cancelled.
  void FinishQuery(uint64_t query_id,
                   std::optional<CredentialCacheEntry> credential,
                   GenerationCompletedCallback callback,
                   QueryResult result);

  void OnQueryCompleted(std::optional<CredentialCacheEntry> credential,
                        GenerationCompletedCallback callback,
                        QueryResult result);
  void OnQueryDataReceived(GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);

  std::optional<std::string> GetLeoModelKeyFromResponse(
      const base::DictValue& response);

  const std::string model_name_;
  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  // Where streamed events are parsed. Sequenced, so that a reply posted after
  // the parses runs after theirs.
  const scoped_refptr<base::SequencedTaskRunner> decoder_task_runner_;
  base::flat_map<uint64_t, std::unique_ptr<Query>> queries_;
  uint64_t next_query_id_ = 0;
  raw_ptr<AIChatCredentialManager> credential_manager_;
  raw_ptr<ModelService> model_service_;

  base::WeakPtrFactory<ConversationAPIClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_CLIENT_H_
