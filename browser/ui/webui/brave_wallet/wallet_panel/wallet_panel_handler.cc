// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

// It's safe to bind the active webcontents when panel is created because
// the panel will not be shared across tabs.
WalletPanelHandler::WalletPanelHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
    TopChromeWebUIController* webui_controller,
    content::WebContents* active_web_contents)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller),
      active_web_contents_(active_web_contents) {
  DCHECK(active_web_contents_);
}

WalletPanelHandler::~WalletPanelHandler() = default;

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

void WalletPanelHandler::CloseSidePanel() {
  content::WebContents* web_contents =
      webui_controller_->web_ui()->GetWebContents();
  if (!web_contents) {
    return;
  }

  BrowserWindowInterface* browser =
      webui::GetBrowserWindowInterface(web_contents);
  if (!browser) {
    return;
  }

  SidePanelUI* side_panel_ui = browser->GetFeatures().side_panel_ui();
  if (side_panel_ui &&
      side_panel_ui->GetCurrentEntryId() == SidePanelEntryId::kWallet) {
    side_panel_ui->Close();
  }
}

void WalletPanelHandler::ConnectToSite(
    const std::vector<std::string>& accounts,
    brave_wallet::mojom::PermissionLifetimeOption option) {
  permissions::BraveWalletPermissionContext::AcceptOrCancel(
      accounts, option, active_web_contents_);
}

void WalletPanelHandler::CancelConnectToSite() {
  permissions::BraveWalletPermissionContext::Cancel(active_web_contents_);
}

void WalletPanelHandler::Focus() {
  webui_controller_->web_ui()->GetWebContents()->Focus();
}

void WalletPanelHandler::IsSolanaAccountConnected(
    const std::string& account,
    IsSolanaAccountConnectedCallback callback) {
  // Report the connection state of the frame the panel names, not of whichever
  // frame happens to hold focus. See WalletPanelHandler::RequestPermission for
  // the rationale.
  content::RenderFrameHost* rfh = active_web_contents_->GetPrimaryMainFrame();

  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active_web_contents_);
  if (!tab_helper) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(
      tab_helper->IsSolanaAccountConnected(rfh->GetGlobalId(), account));
}

void WalletPanelHandler::RequestPermission(
    brave_wallet::mojom::AccountIdPtr account_id,
    RequestPermissionCallback callback) {
  // The panel names the primary main frame's origin (see
  // BraveWalletServiceDelegateImpl::GetActiveOrigin), and so do the connected
  // accounts list and Disconnect. Grant to that same frame: using the focused
  // frame would let a cross-origin subframe holding focus receive a durable
  // permission the user was never shown and cannot revoke from this panel.
  content::RenderFrameHost* rfh = active_web_contents_->GetPrimaryMainFrame();

  auto request_type =
      brave_wallet::CoinTypeToPermissionRequestType(account_id->coin);
  auto permission = brave_wallet::CoinTypeToPermissionType(account_id->coin);
  if (!request_type || !permission) {
    std::move(callback).Run(false);
    return;
  }

  if (permissions::BraveWalletPermissionContext::HasRequestsInProgress(
          rfh, *request_type)) {
    std::move(callback).Run(false);
    return;
  }

  auto address = brave_wallet::GetAccountPermissionIdentifier(account_id);

  permissions::BraveWalletPermissionContext::RequestWalletPermissions(
      {address}, *permission, rfh->GetLastCommittedOrigin(), rfh,
      base::BindOnce(
          [](RequestPermissionCallback cb, std::string address,
             std::vector<std::string> allowed_addresses) {
            std::move(cb).Run(allowed_addresses.size() == 1 &&
                              allowed_addresses.front() == address);
          },
          std::move(callback), address));
}
