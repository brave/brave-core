// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/wallet_panel/wallet_panel_page_handler.h"

#include <utility>

#include "content/public/browser/web_ui.h"

WalletPanelPageHandler::WalletPanelPageHandler(
    mojo::PendingReceiver<wallet_panel::mojom::PageHandler> receiver,
    mojo::PendingRemote<wallet_panel::mojom::Page> page,
    content::WebUI* web_ui,
    ui::MojoBubbleWebUIController* webui_controller)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      web_ui_(web_ui),
      webui_controller_(webui_controller) {
  Observe(web_ui_->GetWebContents());
}

WalletPanelPageHandler::~WalletPanelPageHandler() = default;

void WalletPanelPageHandler::ShowUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->ShowUI();
  }
}

void WalletPanelPageHandler::CloseUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}

void WalletPanelPageHandler::OnVisibilityChanged(
    content::Visibility visibility) {
  webui_hidden_ = visibility == content::Visibility::HIDDEN;
}
