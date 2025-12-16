// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_

#include <memory>

#include "brave/components/local_ai/common/local_ai_internals.mojom.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace local_ai {
class CandleService;

class LocalAIInternalsPageHandler
    : public local_ai_internals::mojom::PageHandler {
 public:
  explicit LocalAIInternalsPageHandler(
      mojo::PendingReceiver<local_ai_internals::mojom::PageHandler> receiver,
      CandleService* candle_service);
  ~LocalAIInternalsPageHandler() override;

  LocalAIInternalsPageHandler(const LocalAIInternalsPageHandler&) = delete;
  LocalAIInternalsPageHandler& operator=(const LocalAIInternalsPageHandler&) =
      delete;

  // local_ai_internals::mojom::PageHandler:
  void GenerateEmbedding(const std::string& text,
                         GenerateEmbeddingCallback callback) override;

 private:
  mojo::Receiver<local_ai_internals::mojom::PageHandler> receiver_;
  raw_ptr<CandleService> candle_service_;
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
  raw_ptr<CandleService> candle_service_;
};

class LocalAIInternalsUIConfig
    : public content::DefaultWebUIConfig<LocalAIInternalsUI> {
 public:
  LocalAIInternalsUIConfig();
  ~LocalAIInternalsUIConfig() override = default;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_
