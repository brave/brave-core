// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/panel_handler/wallet_panel_handler.h"

#include <utility>

#include "base/callback.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"

WalletPanelHandler::WalletPanelHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
    ui::MojoBubbleWebUIController* webui_controller,
    GetActiveWebContentsCallback get_active_web_contents,
    PanelCloseOnDeactivationCallback close_on_deactivation)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller),
      get_active_web_contents_(std::move(get_active_web_contents)),
      close_on_deactivation_(std::move(close_on_deactivation)) {}

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
                                       const std::string& origin) {
  content::WebContents* contents = get_active_web_contents_.Run();
  if (!contents)
    return;

  permissions::BraveEthereumPermissionContext::AcceptOrCancel(accounts,
                                                              contents);
}

void WalletPanelHandler::CancelConnectToSite(const std::string& origin) {
  content::WebContents* contents = get_active_web_contents_.Run();
  if (!contents)
    return;

  permissions::BraveEthereumPermissionContext::Cancel(contents);
}

void WalletPanelHandler::SetCloseOnDeactivate(bool close) {
  if (close_on_deactivation_)
    close_on_deactivation_.Run(close);
}
