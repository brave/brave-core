/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_CLIENT_H_

#include <memory>
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
  struct ConnectionResult {
    bool connected = false;
    std::string error;
  };

  using ConnectionCallback = base::OnceCallback<void(ConnectionResult)>;
  using ModelsCallback = base::OnceCallback<void(std::string response_body)>;

  explicit OllamaClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OllamaClient() override;

  OllamaClient(const OllamaClient&) = delete;
  OllamaClient& operator=(const OllamaClient&) = delete;

  // Bind a receiver for the OllamaService interface
  void BindReceiver(mojo::PendingReceiver<mojom::OllamaService> receiver);

  // mojom::OllamaService implementation:
  void CheckConnection(CheckConnectionCallback callback) override;

  // Fetch available models from Ollama (non-mojo method for internal use)
  void FetchModels(ModelsCallback callback);

 private:
  void OnConnectionCheckComplete(
      CheckConnectionCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      std::unique_ptr<std::string> response);

  void OnModelsListComplete(ModelsCallback callback,
                            std::unique_ptr<network::SimpleURLLoader> loader,
                            std::unique_ptr<std::string> response);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::ReceiverSet<mojom::OllamaService> receivers_;
  base::WeakPtrFactory<OllamaClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_CLIENT_H_
