// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/panel_handler/wallet_panel_handler.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "components/permissions/permission_request_manager.h"

WalletPanelHandler::WalletPanelHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
    mojo::PendingRemote<brave_wallet::mojom::Page> page,
    content::WebUI* web_ui,
    ui::MojoBubbleWebUIController* webui_controller,
    GetWebContentsForTabCallback get_web_contents_for_tab)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      web_ui_(web_ui),
      webui_controller_(webui_controller),
      get_web_contents_for_tab_(std::move(get_web_contents_for_tab)) {
  wallet_service_ =
      brave_wallet::BraveWalletServiceFactory::GetInstance()->GetForContext(
          web_ui_->GetWebContents()->GetBrowserContext());
  wallet_service_->AddObserver(this);
  wallet_service_->SetPanelHandlerReady(true);
}

WalletPanelHandler::~WalletPanelHandler() {
  wallet_service_->SetPanelHandlerReady(false);
  wallet_service_->RemoveObserver(this);
}

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

  permissions::PermissionRequestManager* manager =
      permissions::PermissionRequestManager::FromWebContents(contents);
  DCHECK(manager);

  manager->AcceptEthereumPermissionRequests(accounts);
}

void WalletPanelHandler::CancelConnectToSite(const std::string& origin,
                                             int32_t tab_id) {
  content::WebContents* contents = get_web_contents_for_tab_.Run(tab_id);
  if (!contents)
    return;

  permissions::PermissionRequestManager* manager =
      permissions::PermissionRequestManager::FromWebContents(contents);
  DCHECK(manager);

  manager->IgnoreEthereumPermissionRequests();
}

void WalletPanelHandler::OnShowEthereumPermissionPrompt(
    int32_t tab_id,
    const std::vector<std::string>& accounts,
    const std::string& origin) {
  page_->ShowConnectToSiteUI(tab_id, accounts, origin);
}
