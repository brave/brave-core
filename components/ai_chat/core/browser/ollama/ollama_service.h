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
class OllamaService : public KeyedService, public mojom::OllamaService {
 public:
  explicit OllamaService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OllamaService() override;

  OllamaService(const OllamaService&) = delete;
  OllamaService& operator=(const OllamaService&) = delete;

  // Bind a receiver for the OllamaService interface
  void BindReceiver(mojo::PendingReceiver<mojom::OllamaService> receiver);

  // mojom::OllamaService implementation:
  void IsConnected(IsConnectedCallback callback) override;

 private:
  void OnConnectionCheckComplete(
      IsConnectedCallback callback,
      std::unique_ptr<network::SimpleURLLoader> loader,
      std::optional<std::string> response);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::ReceiverSet<mojom::OllamaService> receivers_;
  base::WeakPtrFactory<OllamaService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_OLLAMA_SERVICE_H_
