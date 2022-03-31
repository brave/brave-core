// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace content {
class WebUI;
}

class WalletPanelHandler : public brave_wallet::mojom::PanelHandler {
 public:
  using GetActiveWebContentsCallback =
      base::RepeatingCallback<content::WebContents*()>;
  using PanelCloseOnDeactivationCallback = base::RepeatingCallback<void(bool)>;
  WalletPanelHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
      ui::MojoBubbleWebUIController* webui_controller,
      GetActiveWebContentsCallback get_active_web_contents,
      PanelCloseOnDeactivationCallback close_on_deactivation);

  WalletPanelHandler(const WalletPanelHandler&) = delete;
  WalletPanelHandler& operator=(const WalletPanelHandler&) = delete;
  ~WalletPanelHandler() override;

  // brave_wallet::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;
  void SetCloseOnDeactivate(bool close) override;
  void ConnectToSite(const std::vector<std::string>& accounts,
                     const std::string& origin) override;
  void CancelConnectToSite(const std::string& origin) override;

 private:
  mojo::Receiver<brave_wallet::mojom::PanelHandler> receiver_;
  ui::MojoBubbleWebUIController* const webui_controller_;
  const GetActiveWebContentsCallback get_active_web_contents_;
  const PanelCloseOnDeactivationCallback close_on_deactivation_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
