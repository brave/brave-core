/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_HISTORY_BRAVE_HISTORY_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_HISTORY_BRAVE_HISTORY_UI_H_

#include <memory>

#include "brave/browser/ui/webui/history/brave_history_embeddings.mojom.h"
#include "chrome/browser/ui/webui/history/history_ui.h"
#include "chrome/common/url_constants.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class BraveHistoryEmbeddingsPageHandler;
class BraveHistoryUI;

class BraveHistoryUIConfig
    : public content::DefaultWebUIConfig<BraveHistoryUI> {
 public:
  BraveHistoryUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme,
                           chrome::kChromeUIHistoryHost) {}
};

// Brave subclass of the chrome://history WebUI controller that adds a
// Mojo interface (Semantic History Search toggle handler) on top of
// upstream's bindings.
class BraveHistoryUI
    : public HistoryUI,
      public brave_history_embeddings::mojom::PageHandlerFactory {
 public:
  explicit BraveHistoryUI(content::WebUI* web_ui);
  BraveHistoryUI(const BraveHistoryUI&) = delete;
  BraveHistoryUI& operator=(const BraveHistoryUI&) = delete;
  ~BraveHistoryUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandlerFactory>
          receiver);

  // brave_history_embeddings::mojom::PageHandlerFactory:
  void CreateInterfacePageHandler(
      mojo::PendingRemote<brave_history_embeddings::mojom::Page> page,
      mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
          receiver) override;

 private:
  mojo::Receiver<brave_history_embeddings::mojom::PageHandlerFactory>
      page_handler_factory_receiver_{this};
  std::unique_ptr<BraveHistoryEmbeddingsPageHandler> page_handler_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_HISTORY_BRAVE_HISTORY_UI_H_
