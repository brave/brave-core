/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_CLIENT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
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
class OllamaClient : public KeyedService, public mojom::OllamaService {
 public:
  struct ModelInfo {
    std::string name;
    ModelInfo();
    ModelInfo(const ModelInfo&);
    ModelInfo& operator=(const ModelInfo&);
    ModelInfo(ModelInfo&&);
    ModelInfo& operator=(ModelInfo&&);
    ~ModelInfo();
  };

  struct ModelDetails {
    uint32_t context_length = 0;
    bool has_vision = false;
    ModelDetails();
    ModelDetails(const ModelDetails&);
    ModelDetails& operator=(const ModelDetails&);
    ModelDetails(ModelDetails&&);
    ModelDetails& operator=(ModelDetails&&);
    ~ModelDetails();
  };

  using ModelsCallback =
      base::OnceCallback<void(std::optional<std::vector<ModelInfo>>)>;
  using ModelDetailsCallback =
      base::OnceCallback<void(std::optional<ModelDetails>)>;

  explicit OllamaClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OllamaClient() override;

  OllamaClient(const OllamaClient&) = delete;
  OllamaClient& operator=(const OllamaClient&) = delete;

  // Bind a receiver for the OllamaService interface
  void BindReceiver(mojo::PendingReceiver<mojom::OllamaService> receiver);

  // mojom::OllamaService implementation:
  void Connected(ConnectedCallback callback) override;

  // Fetch available models from Ollama (non-mojo method for internal use)
  void FetchModels(ModelsCallback callback);

  // Fetch detailed information for a specific model
  void ShowModel(const std::string& model_name, ModelDetailsCallback callback);

 private:
  void OnConnectionCheckComplete(
      ConnectedCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      std::optional<std::string> response);

  void OnModelsListComplete(ModelsCallback callback,
                            std::unique_ptr<network::SimpleURLLoader> loader,
                            std::optional<std::string> response);

  void OnModelDetailsComplete(ModelDetailsCallback callback,
                              std::unique_ptr<network::SimpleURLLoader> loader,
                              std::optional<std::string> response);

  std::optional<std::vector<ModelInfo>> ParseModelsResponse(
      const std::string& response_body);
  std::optional<ModelDetails> ParseModelDetailsResponse(
      const std::string& response_body);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::ReceiverSet<mojom::OllamaService> receivers_;
  base::WeakPtrFactory<OllamaClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_CLIENT_H_
