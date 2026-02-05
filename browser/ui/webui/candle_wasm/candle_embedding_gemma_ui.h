// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_CANDLE_WASM_CANDLE_EMBEDDING_GEMMA_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_CANDLE_WASM_CANDLE_EMBEDDING_GEMMA_UI_H_

#include "brave/components/local_ai/core/local_ai.mojom-forward.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace local_ai {

// WebUI controller for the chrome-untrusted:// page that hosts the
// Gemma embedding model compiled to WebAssembly via the Candle ML
// framework. Runs in an isolated (untrusted) renderer process since
// it processes arbitrary page text for embedding generation.
class UntrustedCandleEmbeddingGemmaUI : public ui::MojoWebUIController {
 public:
  explicit UntrustedCandleEmbeddingGemmaUI(content::WebUI* web_ui);
  UntrustedCandleEmbeddingGemmaUI(const UntrustedCandleEmbeddingGemmaUI&) =
      delete;
  UntrustedCandleEmbeddingGemmaUI& operator=(
      const UntrustedCandleEmbeddingGemmaUI&) = delete;
  ~UntrustedCandleEmbeddingGemmaUI() override;

  void BindInterface(mojo::PendingReceiver<mojom::LocalAIService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

// Registers the chrome-untrusted://on-device-model-worker/ URL and
// creates UntrustedCandleEmbeddingGemmaUI instances for it.
class UntrustedCandleEmbeddingGemmaUIConfig : public content::WebUIConfig {
 public:
  UntrustedCandleEmbeddingGemmaUIConfig();
  ~UntrustedCandleEmbeddingGemmaUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_CANDLE_WASM_CANDLE_EMBEDDING_GEMMA_UI_H_
