// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_ai_internals.mojom.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace local_ai {

class LocalAIInternalsPageHandler
    : public local_ai_internals::mojom::PageHandler {
 public:
  explicit LocalAIInternalsPageHandler(
      mojo::PendingReceiver<local_ai_internals::mojom::PageHandler> receiver,
      mojo::PendingRemote<mojom::LocalAIService> local_ai_service);
  ~LocalAIInternalsPageHandler() override;

  LocalAIInternalsPageHandler(const LocalAIInternalsPageHandler&) = delete;
  LocalAIInternalsPageHandler& operator=(const LocalAIInternalsPageHandler&) =
      delete;

  // local_ai_internals::mojom::PageHandler:
  void GenerateEmbedding(const std::string& text,
                         GenerateEmbeddingCallback callback) override;

 private:
  void OnPassageEmbedderReady(
      mojo::PendingRemote<mojom::PassageEmbedder> remote);
  void OnEmbeddingResult(const std::vector<double>& embedding);
  void OnPassageEmbedderDisconnected();
  void OnIdleTimeout();
  void CancelAllPending();

  mojo::Receiver<local_ai_internals::mojom::PageHandler> receiver_;
  mojo::Remote<mojom::LocalAIService> local_ai_service_;
  mojo::Remote<mojom::PassageEmbedder> passage_embedder_;

  // Mojo's set_idle_handler() relies on the receiver sending MessageAck
  // and NotifyIdle control messages, but the JS Mojo bindings don't
  // implement this protocol. Use an explicit timer instead.
  base::OneShotTimer idle_timer_;

  // Texts queued while waiting for the embedder to bind.
  std::vector<std::string> pending_texts_;
  // Response callbacks in FIFO order matching the request order.
  std::vector<GenerateEmbeddingCallback> pending_callbacks_;

  base::WeakPtrFactory<LocalAIInternalsPageHandler> weak_ptr_factory_{this};
};

// Trusted WebUI at chrome://local-ai-internals
class LocalAIInternalsUI : public ui::MojoWebUIController {
 public:
  explicit LocalAIInternalsUI(content::WebUI* web_ui);
  LocalAIInternalsUI(const LocalAIInternalsUI&) = delete;
  LocalAIInternalsUI& operator=(const LocalAIInternalsUI&) = delete;
  ~LocalAIInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<local_ai_internals::mojom::PageHandler> receiver);

  WEB_UI_CONTROLLER_TYPE_DECL();

 private:
  std::unique_ptr<LocalAIInternalsPageHandler> page_handler_;
};

class LocalAIInternalsUIConfig
    : public content::DefaultWebUIConfig<LocalAIInternalsUI> {
 public:
  LocalAIInternalsUIConfig();
  ~LocalAIInternalsUIConfig() override = default;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_
