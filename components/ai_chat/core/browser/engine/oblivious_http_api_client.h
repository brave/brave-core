// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OBLIVIOUS_HTTP_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OBLIVIOUS_HTTP_API_CLIENT_H_

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/oblivious_http_config_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Performs OAI-compatible chat completion requests over OHTTP. Routes the
// inner HTTP request through the AI Chat OHTTP relay (the AI Chat server),
// using a cached HPKE key config managed by ObliviousHttpConfigManager. The Leo
// SKU credential, the Brave services key, and the model name are passed as
// outer (relay) request headers.
//
class ObliviousHttpAPIClient : public OAIAPIClient {
 public:
  ObliviousHttpAPIClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      network::NetworkContextGetter network_context_getter,
      AIChatCredentialManager* credential_manager,
      PrefService* profile_prefs);

  ObliviousHttpAPIClient(const ObliviousHttpAPIClient&) = delete;
  ObliviousHttpAPIClient& operator=(const ObliviousHttpAPIClient&) = delete;
  ~ObliviousHttpAPIClient() override;

  // OAIAPIClient:
  // |model_options| must hold a LeoModelOptions variant.
  void PerformRequest(const mojom::ModelOptions& model_options,
                      std::vector<OAIMessage> messages,
                      std::optional<base::ListValue> oai_tool_definitions,
                      GenerationDataCallback data_received_callback,
                      GenerationCompletedCallback completed_callback,
                      const std::optional<std::vector<std::string>>&
                          stop_sequences = std::nullopt) override;

  void ClearAllQueries() override;

  void SetConfigManagerForTesting(
      std::unique_ptr<ObliviousHttpConfigManager> config_manager) {
    config_manager_ = std::move(config_manager);
  }

 private:
  // Self-managed mojo client that receives both body chunks
  // (ObliviousHttpChunkClient) and the final completion (ObliviousHttpClient)
  // for a single OHTTP request. Owned by ObliviousHttpAPIClient via
  // inner_clients_. The owning list entry is erased once the dispatch callback
  // fires (from either OnCompleted or a pipe disconnect).
  class InnerClient : public network::mojom::ObliviousHttpClient,
                      public network::mojom::ObliviousHttpChunkClient {
   public:
    using CompletionCallback =
        base::OnceCallback<void(int outer_response_code,
                                int inner_response_code,
                                std::string response_body)>;
    using ChunkCallback = base::RepeatingCallback<void(std::string chunk)>;

    InnerClient(CompletionCallback completion_callback,
                ChunkCallback chunk_callback);

    InnerClient(const InnerClient&) = delete;
    InnerClient& operator=(const InnerClient&) = delete;

    ~InnerClient() override;

    // Returns a pending_remote bound to the completion receiver.
    mojo::PendingRemote<network::mojom::ObliviousHttpClient>
    BindCompletionReceiver();

    // Returns a pending_remote bound to the chunk receiver. Only call this
    // when chunking is enabled.
    mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
    BindChunkReceiver();

    // network::mojom::ObliviousHttpClient:
    void OnCompleted(
        network::mojom::ObliviousHttpCompletionResultPtr response) override;

    // network::mojom::ObliviousHttpChunkClient:
    void OnBodyChunk(const std::string& chunk) override;

   private:
    void OnPipeDisconnected();

    CompletionCallback completion_callback_;
    ChunkCallback chunk_callback_;

    mojo::Receiver<network::mojom::ObliviousHttpClient> completion_receiver_{
        this};
    mojo::Receiver<network::mojom::ObliviousHttpChunkClient> chunk_receiver_{
        this};
  };

  using InnerClientList = std::list<std::unique_ptr<InnerClient>>;

  struct Request {
    Request(std::string model_name,
            std::string request_body,
            GenerationDataCallback data_received_callback,
            GenerationCompletedCallback completed_callback);
    Request(Request&&);
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;
    ~Request();

    std::string model_name;
    std::string request_body;
    GenerationDataCallback data_received_callback;
    GenerationCompletedCallback completed_callback;
    // Set in DispatchOHTTPRequest once the client is inserted into the list.
    InnerClientList::iterator it;
  };

  // Per-request helpers.
  void OnCredentialFetched(Request request,
                           std::optional<CredentialCacheEntry> credential);
  void OnKeyConfigReady(
      Request request,
      std::optional<CredentialCacheEntry> credential,
      std::optional<ObliviousHttpConfigManager::KeyConfigResult>
          key_config_result);
  void DispatchOHTTPRequest(
      ObliviousHttpConfigManager::KeyConfigResult key_config_result,
      Request request,
      std::optional<CredentialCacheEntry> credential);
  void OnInnerResponse(Request request,
                       std::optional<CredentialCacheEntry> credential,
                       int outer_response_code,
                       int inner_response_code,
                       std::string response_body);
  void OnInnerChunkReceived(GenerationDataCallback data_received_callback,
                            std::string model_name,
                            std::string chunk);
  static void OnChunkParsed(GenerationDataCallback data_received_callback,
                            std::string model_name,
                            std::optional<base::Value> value);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  network::NetworkContextGetter network_context_getter_;
  raw_ptr<AIChatCredentialManager> credential_manager_;

  std::unique_ptr<ObliviousHttpConfigManager> config_manager_;
  InnerClientList inner_clients_;
  scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner_;

  base::WeakPtrFactory<ObliviousHttpAPIClient> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OBLIVIOUS_HTTP_API_CLIENT_H_
