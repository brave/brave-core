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

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Handles network communication with a local Ollama instance.
class OllamaClient {
 public:
  struct ConnectionResult {
    bool connected = false;
    std::string error;
  };

  using ConnectionCallback = base::OnceCallback<void(ConnectionResult)>;
  using ModelsCallback = base::OnceCallback<void(std::string response_body)>;

  explicit OllamaClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OllamaClient();

  OllamaClient(const OllamaClient&) = delete;
  OllamaClient& operator=(const OllamaClient&) = delete;

  // Check if Ollama is running at localhost:11434
  void CheckConnection(ConnectionCallback callback);

  // Fetch available models from Ollama
  void FetchModels(ModelsCallback callback);

 private:
  void OnConnectionCheckComplete(
      ConnectionCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      std::unique_ptr<std::string> response);

  void OnModelsListComplete(ModelsCallback callback,
                            std::unique_ptr<network::SimpleURLLoader> loader,
                            std::unique_ptr<std::string> response);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<OllamaClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_CLIENT_H_
