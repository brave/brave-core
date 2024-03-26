// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class WebUI;
}

class WalletPanelHandler : public brave_wallet::mojom::PanelHandler {
 public:
  using PanelCloseOnDeactivationCallback = base::RepeatingCallback<void(bool)>;
  WalletPanelHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
      TopChromeWebUIController* webui_controller,
      content::WebContents* active_web_contents,
      PanelCloseOnDeactivationCallback close_on_deactivation);

  WalletPanelHandler(const WalletPanelHandler&) = delete;
  WalletPanelHandler& operator=(const WalletPanelHandler&) = delete;
  ~WalletPanelHandler() override;

  // brave_wallet::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;
  void SetCloseOnDeactivate(bool close) override;
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

 private:
  mojo::Receiver<brave_wallet::mojom::PanelHandler> receiver_;
  const raw_ptr<TopChromeWebUIController> webui_controller_;
  raw_ptr<content::WebContents> active_web_contents_ = nullptr;
  const PanelCloseOnDeactivationCallback close_on_deactivation_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
