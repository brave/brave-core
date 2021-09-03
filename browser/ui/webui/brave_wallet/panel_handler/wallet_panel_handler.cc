// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/panel_handler/wallet_panel_handler.h"

#include <utility>

#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"

WalletPanelHandler::WalletPanelHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
    ui::MojoBubbleWebUIController* webui_controller,
    GetWebContentsForTabCallback get_web_contents_for_tab)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller),
      get_web_contents_for_tab_(std::move(get_web_contents_for_tab)) {}

WalletPanelHandler::~WalletPanelHandler() {}

void WalletPanelHandler::ShowUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->ShowUI();
  }
}

void WalletPanelHandler::CloseUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}

void WalletPanelHandler::ConnectToSite(const std::vector<std::string>& accounts,
                                       const std::string& origin,
                                       int32_t tab_id) {
  content::WebContents* contents = get_web_contents_for_tab_.Run(tab_id);
  if (!contents)
    return;

  permissions::BraveEthereumPermissionContext::AcceptOrCancel(accounts,
                                                              contents);
}

void WalletPanelHandler::CancelConnectToSite(const std::string& origin,
                                             int32_t tab_id) {
  content::WebContents* contents = get_web_contents_for_tab_.Run(tab_id);
  if (!contents)
    return;

  permissions::BraveEthereumPermissionContext::Cancel(contents);
}
