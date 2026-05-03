// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OHTTP_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OHTTP_API_CLIENT_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/ohttp_config_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Performs OAI-compatible chat completion requests over OHTTP. Routes the
// inner HTTP request through the AI Chat OHTTP relay (the AI Chat server),
// using a cached HPKE key config managed by OHTTPConfigManager. The Leo SKU
// credential, the Brave services key, and the model name are passed as outer
// (relay) request headers.
//
class OHTTPAPIClient : public OAIAPIClient {
 public:
  OHTTPAPIClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      network::NetworkContextGetter network_context_getter,
      AIChatCredentialManager* credential_manager,
      PrefService* profile_prefs);

  OHTTPAPIClient(const OHTTPAPIClient&) = delete;
  OHTTPAPIClient& operator=(const OHTTPAPIClient&) = delete;
  ~OHTTPAPIClient() override;

  // OAIAPIClient:
  // |model_options| must hold a LeoModelOptions variant.
  void PerformRequest(
      const mojom::ModelOptions& model_options,
      std::vector<OAIMessage> messages,
      std::optional<base::ListValue> oai_tool_definitions,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      const std::optional<std::vector<std::string>>& stop_sequences =
          std::nullopt) override;

  void ClearAllQueries() override;

 private:
  // Self-owned mojo client receiver for the network service ObliviousHttp
  // pipe. Lives until the network service either delivers OnCompleted or
  // closes the pipe.
  class InnerOhttpClient : public network::mojom::ObliviousHttpClient {
   public:
    using DispatchCallback =
        base::OnceCallback<void(int response_code, std::string response_body)>;

    explicit InnerOhttpClient(DispatchCallback callback);

    InnerOhttpClient(const InnerOhttpClient&) = delete;
    InnerOhttpClient& operator=(const InnerOhttpClient&) = delete;

    ~InnerOhttpClient() override;

    // network::mojom::ObliviousHttpClient:
    void OnCompleted(
        network::mojom::ObliviousHttpCompletionResultPtr response) override;

   private:
    DispatchCallback callback_;
  };

  struct Request {
    Request(std::string model_name,
            std::string request_body,
            GenerationCompletedCallback completed_callback);
    Request(Request&&);
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;
    ~Request();

    std::string model_name;
    std::string request_body;
    GenerationCompletedCallback completed_callback;
  };

  // Per-request helpers.
  void OnCredentialFetched(Request request,
                           std::optional<CredentialCacheEntry> credential);
  void OnKeyConfigReady(
      Request request,
      std::optional<CredentialCacheEntry> credential,
      std::optional<OHTTPConfigManager::KeyConfigResult> key_config_result);
  void DispatchOhttpRequest(
      OHTTPConfigManager::KeyConfigResult key_config_result,
      Request request,
      std::optional<CredentialCacheEntry> credential);
  void OnInnerResponse(Request request,
                       std::optional<CredentialCacheEntry> credential,
                       int response_code,
                       std::string response_body);
  void RunCompletedWithError(GenerationCompletedCallback completed_callback,
                             mojom::APIError error);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  network::NetworkContextGetter network_context_getter_;
  raw_ptr<AIChatCredentialManager> credential_manager_;

  std::unique_ptr<OHTTPConfigManager> config_manager_;

  base::WeakPtrFactory<OHTTPAPIClient> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OHTTP_API_CLIENT_H_
