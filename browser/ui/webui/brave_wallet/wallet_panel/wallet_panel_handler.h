// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_WALLET_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_WALLET_PANEL_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/polkadot_bridge.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace brave_wallet {
class PolkadotWalletService;
}  // namespace brave_wallet

class WalletPanelHandler : public brave_wallet::mojom::PanelHandler,
                           public content::WebContentsObserver {
 public:
  using PanelCloseOnDeactivationCallback = base::RepeatingCallback<void(bool)>;
  WalletPanelHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
      TopChromeWebUIController* webui_controller,
      content::WebContents* active_web_contents);

  WalletPanelHandler(const WalletPanelHandler&) = delete;
  WalletPanelHandler& operator=(const WalletPanelHandler&) = delete;
  ~WalletPanelHandler() override;

  // brave_wallet::mojom::PanelHandler:
  void AddObserver(
      mojo::PendingRemote<brave_wallet::mojom::PanelHandlerObserver> observer)
      override;
  void GetActivePanelOrigin(GetActivePanelOriginCallback callback) override;
  void ShowUI() override;
  void CloseUI() override;
  void CloseSidePanel() override;
  void ConnectToSite(
      const std::vector<std::string>& accounts,
      brave_wallet::mojom::PermissionLifetimeOption option) override;
  void CancelConnectToSite() override;
  void Focus() override;
  void IsSolanaAccountConnected(
      const std::string& account,
      IsSolanaAccountConnectedCallback callback) override;
  void RequestPermission(brave_wallet::mojom::AccountIdPtr account_id,
                         RequestPermissionCallback callback) override;
  void GetPolkadotSignRequestDetails(
      int32_t request_id,
      GetPolkadotSignRequestDetailsCallback callback) override;

 private:
  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;

  brave_wallet::PolkadotWalletService* GetPolkadotWalletService();

  void DescribePolkadotSignRequest(
      int32_t request_id,
      GetPolkadotSignRequestDetailsCallback callback,
      bool bridge_ready);

  void OnPolkadotPayloadDecoded(
      int32_t request_id,
      GetPolkadotSignRequestDetailsCallback callback,
      brave_wallet::mojom::PolkadotDecodedPayloadPtr decoded);

  void OnPolkadotFeeEstimated(
      std::string as_human,
      GetPolkadotSignRequestDetailsCallback callback,
      base::expected<brave_wallet::uint128_t, std::string> partial_fee);

  mojo::Receiver<brave_wallet::mojom::PanelHandler> receiver_;
  const raw_ptr<TopChromeWebUIController> webui_controller_;
  mojo::RemoteSet<brave_wallet::mojom::PanelHandlerObserver> observers_;
  base::WeakPtrFactory<WalletPanelHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_WALLET_PANEL_HANDLER_H_
