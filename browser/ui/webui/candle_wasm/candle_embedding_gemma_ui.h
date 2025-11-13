// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_CANDLE_WASM_CANDLE_EMBEDDING_GEMMA_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_CANDLE_WASM_CANDLE_EMBEDDING_GEMMA_UI_H_

#include "brave/components/local_ai/common/candle.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace local_ai {

class UntrustedCandleEmbeddingGemmaUI : public ui::MojoWebUIController {
 public:
  explicit UntrustedCandleEmbeddingGemmaUI(content::WebUI* web_ui);
  UntrustedCandleEmbeddingGemmaUI(const UntrustedCandleEmbeddingGemmaUI&) =
      delete;
  UntrustedCandleEmbeddingGemmaUI& operator=(
      const UntrustedCandleEmbeddingGemmaUI&) = delete;
  ~UntrustedCandleEmbeddingGemmaUI() override;

  void BindInterface(mojo::PendingReceiver<mojom::CandleService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

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
