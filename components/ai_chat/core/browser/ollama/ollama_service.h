/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_model_fetcher.h"
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Handles network communication with a local Ollama instance.
// Implements the mojom::OllamaService interface for UI communication.
// Also implements OllamaModelFetcher::Delegate to provide model fetching
// capabilities.
class OllamaService : public KeyedService,
                      public mojom::OllamaService,
                      public OllamaModelFetcher::Delegate {
 public:
  using ModelDetails = OllamaModelFetcher::ModelDetails;
  using ModelsCallback = OllamaModelFetcher::Delegate::ModelsCallback;
  using ModelDetailsCallback =
      OllamaModelFetcher::Delegate::ModelDetailsCallback;

  OllamaService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<OllamaModelFetcher> model_fetcher);
  ~OllamaService() override;

  OllamaService(const OllamaService&) = delete;
  OllamaService& operator=(const OllamaService&) = delete;

  // Bind a receiver for the OllamaService interface
  void BindReceiver(mojo::PendingReceiver<mojom::OllamaService> receiver);

  // KeyedService implementation:
  void Shutdown() override;

  // mojom::OllamaService implementation:
  void IsConnected(IsConnectedCallback callback) override;

  // OllamaModelFetcher::Delegate implementation:
  void FetchModels(ModelsCallback callback) override;
  void ShowModel(const std::string& model_name,
                 ModelDetailsCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest, ParseModelsResponse_Valid);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest, ParseModelsResponse_InvalidJSON);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest,
                           ParseModelsResponse_MissingModelsKey);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest, ParseModelsResponse_EmptyModels);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest,
                           ParseModelsResponse_InvalidModelStructure);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest, ParseModelDetailsResponse_Valid);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest,
                           ParseModelDetailsResponse_InvalidJSON);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest,
                           ParseModelDetailsResponse_NoModelInfo);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest,
                           ParseModelDetailsResponse_NoCapabilities);
  FRIEND_TEST_ALL_PREFIXES(OllamaServiceTest,
                           ParseModelDetailsResponse_EmptyResponse);

  void OnConnectionCheckComplete(
      IsConnectedCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      std::optional<std::string> response);

  void OnModelsListComplete(ModelsCallback callback,
                            std::unique_ptr<network::SimpleURLLoader> loader,
                            std::optional<std::string> response);

  void OnModelDetailsComplete(ModelDetailsCallback callback,
                              std::unique_ptr<network::SimpleURLLoader> loader,
                              std::optional<std::string> response);

  std::optional<std::vector<std::string>> ParseModelsResponse(
      const std::string& response_body);
  std::optional<ModelDetails> ParseModelDetailsResponse(
      const std::string& response_body);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::ReceiverSet<mojom::OllamaService> receivers_;
  std::unique_ptr<OllamaModelFetcher> model_fetcher_;
  base::WeakPtrFactory<OllamaService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_SERVICE_H_
