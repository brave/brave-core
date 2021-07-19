// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_service_observer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace content {
class WebUI;
}

namespace brave_wallet {
class BraveWalletService;
}

class WalletPanelHandler : public brave_wallet::mojom::PanelHandler,
                           public brave_wallet::BraveWalletServiceObserver {
 public:
  using GetWebContentsForTabCallback =
      base::RepeatingCallback<content::WebContents*(int32_t)>;

  WalletPanelHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
      mojo::PendingRemote<brave_wallet::mojom::Page> page,
      content::WebUI* web_ui,
      ui::MojoBubbleWebUIController* webui_controller,
      GetWebContentsForTabCallback get_web_contents_for_tab);

  WalletPanelHandler(const WalletPanelHandler&) = delete;
  WalletPanelHandler& operator=(const WalletPanelHandler&) = delete;
  ~WalletPanelHandler() override;

  // brave_wallet::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;
  void ConnectToSite(const std::vector<std::string>& accounts,
                     const std::string& origin,
                     int32_t tab_id) override;
  void CancelConnectToSite(const std::string& origin, int32_t tab_id) override;

  // brave_wallet::BraveWalletServiceObserver:
  void OnShowEthereumPermissionPrompt(int32_t tab_id,
                                      const std::vector<std::string>& accounts,
                                      const std::string& origin) override;

 private:
  mojo::Receiver<brave_wallet::mojom::PanelHandler> receiver_;
  mojo::Remote<brave_wallet::mojom::Page> page_;
  content::WebUI* const web_ui_;
  ui::MojoBubbleWebUIController* const webui_controller_;
  brave_wallet::BraveWalletService* wallet_service_;
  const GetWebContentsForTabCallback get_web_contents_for_tab_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
