// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace content {
class WebUI;
}

class WalletPanelHandler : public brave_wallet::mojom::PanelHandler {
 public:
  WalletPanelHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> receiver,
      ui::MojoBubbleWebUIController* webui_controller);

  WalletPanelHandler(const WalletPanelHandler&) = delete;
  WalletPanelHandler& operator=(const WalletPanelHandler&) = delete;
  ~WalletPanelHandler() override;

  // brave_wallet::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;

 private:
  mojo::Receiver<brave_wallet::mojom::PanelHandler> receiver_;
  ui::MojoBubbleWebUIController* const webui_controller_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
