// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_ui.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

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

  SidePanelUI* side_panel_ui = SidePanelUI::From(browser);
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

brave_wallet::BraveWalletService* WalletPanelHandler::GetWalletService() {
  if (!webui_controller_) {
    return nullptr;
  }

  auto* profile = Profile::FromWebUI(webui_controller_->web_ui());
  if (!profile) {
    return nullptr;
  }

  return brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
}

void WalletPanelHandler::GetPolkadotSignRequestDetails(
    int32_t request_id,
    GetPolkadotSignRequestDetailsCallback callback) {
  // The bridge remote hangs off the controller, which tests that drive this
  // handler standalone leave null.
  auto* panel =
      webui_controller_ ? webui_controller_->GetAs<WalletPanelUI>() : nullptr;
  if (!panel) {
    std::move(callback).Run(nullptr);
    return;
  }

  // The frame is created when the panel mounts but only binds once its bundle
  // has evaluated, which a request arriving with the panel routinely beats.
  panel->WaitForPolkadotBridge(base::BindOnce(
      &WalletPanelHandler::DescribePolkadotSignRequest,
      weak_ptr_factory_.GetWeakPtr(), request_id, std::move(callback)));
}

void WalletPanelHandler::DescribePolkadotSignRequest(
    int32_t request_id,
    GetPolkadotSignRequestDetailsCallback callback,
    bool bridge_ready) {
  auto* panel =
      webui_controller_ ? webui_controller_->GetAs<WalletPanelUI>() : nullptr;
  auto* bridge = panel ? panel->GetPolkadotBridge() : nullptr;
  auto* wallet_service = GetWalletService();
  if (!bridge_ready || !bridge || !wallet_service) {
    LOG(ERROR) << "No Polkadot bridge frame bound; cannot describe request";
    std::move(callback).Run(nullptr);
    return;
  }

  // Read the payload out of the queue rather than accepting it from the panel,
  // so what gets described can only ever be what gets signed.
  auto request =
      wallet_service->GetPendingSignPolkadotTransactionRequest(request_id);
  if (!request) {
    std::move(callback).Run(nullptr);
    return;
  }

  // A frame crash mid-decode would otherwise drop the reply and leave the panel
  // waiting forever on a Sign button it never ungates.
  bridge->Decode(
      request->metadata_bytes, request->chain_properties.Clone(),
      request->raw_payload_json,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&WalletPanelHandler::OnPolkadotPayloadDecoded,
                         weak_ptr_factory_.GetWeakPtr(), request_id,
                         std::move(callback)),
          nullptr));
}

void WalletPanelHandler::OnPolkadotPayloadDecoded(
    int32_t request_id,
    GetPolkadotSignRequestDetailsCallback callback,
    brave_wallet::mojom::PolkadotDecodedPayloadPtr decoded) {
  if (!decoded) {
    std::move(callback).Run(nullptr);
    return;
  }

  // The frame reports decode failures in-band, with every half left null. There
  // is nothing the user could meaningfully approve without a description, so
  // fail closed rather than dereferencing any of them.
  if (!decoded->as_human || !decoded->mock_signed_extrinsic ||
      !decoded->signature_payload) {
    LOG(ERROR) << "Polkadot payload decode failed: "
               << decoded->error.value_or("no reason reported");
    std::move(callback).Run(nullptr);
    return;
  }

  auto* wallet_service = GetWalletService();
  auto request =
      wallet_service
          ? wallet_service->GetPendingSignPolkadotTransactionRequest(request_id)
          : nullptr;
  if (!request) {
    std::move(callback).Run(nullptr);
    return;
  }

  // The frame only has to be trusted to describe a call, but this is the half
  // that gets signed, so pin it to the call the dapp declared: the payload
  // encodes that call first and bare, hence as its own prefix.
  if (!brave_wallet::PolkadotSignaturePayloadMatchesCall(
          *decoded->signature_payload, request->raw_payload_json)) {
    LOG(ERROR) << "Polkadot signature payload does not match the declared call";
    std::move(callback).Run(nullptr);
    return;
  }

  // Recorded before the description is handed back, so the panel can't ungate
  // its Sign button on a request that has nothing to sign.
  wallet_service->SetSignPolkadotTransactionRequestSignaturePayload(
      request_id, std::move(*decoded->signature_payload));

  auto* polkadot_service = wallet_service->GetPolkadotWalletService();
  auto* rpc = polkadot_service ? polkadot_service->GetPolkadotRpc() : nullptr;
  if (!rpc) {
    // Still worth showing the user what they are being asked to sign.
    auto details = brave_wallet::mojom::PolkadotSignRequestDetails::New();
    details->as_human = std::move(*decoded->as_human);
    std::move(callback).Run(std::move(details));
    return;
  }

  auto as_human = std::move(*decoded->as_human);
  rpc->GetPaymentInfo(
      request->chain_id->chain_id, *decoded->mock_signed_extrinsic,
      base::BindOnce(&WalletPanelHandler::OnPolkadotFeeEstimated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(as_human),
                     std::move(callback)));
}

void WalletPanelHandler::OnPolkadotFeeEstimated(
    std::string as_human,
    GetPolkadotSignRequestDetailsCallback callback,
    base::expected<brave_wallet::uint128_t, std::string> partial_fee) {
  auto details = brave_wallet::mojom::PolkadotSignRequestDetails::New();
  details->as_human = std::move(as_human);
  if (partial_fee.has_value()) {
    details->fee = brave_wallet::mojom::PolkadotFeeEstimate::New(
        brave_wallet::Uint128ToMojom(partial_fee.value()));
  }
  std::move(callback).Run(std::move(details));
}
