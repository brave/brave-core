// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_

#include <string>

#include "brave/components/brave_wallet_ui/wallet_ui.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace content {
class WebUI;
}

class WalletPanelHandler : public wallet_ui::mojom::PanelHandler,
                           public content::WebContentsObserver {
 public:
  WalletPanelHandler(
      mojo::PendingReceiver<wallet_ui::mojom::PanelHandler> receiver,
      mojo::PendingRemote<wallet_ui::mojom::Page> page,
      content::WebUI* web_ui,
      ui::MojoBubbleWebUIController* webui_controller);

  WalletPanelHandler(const WalletPanelHandler&) = delete;
  WalletPanelHandler& operator=(const WalletPanelHandler&) = delete;
  ~WalletPanelHandler() override;

  // content::WebContentsObserver:
  void OnVisibilityChanged(content::Visibility visibility) override;

  // wallet_ui::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;

 private:
  bool webui_hidden_ = false;
  mojo::Receiver<wallet_ui::mojom::PanelHandler> receiver_;
  mojo::Remote<wallet_ui::mojom::Page> page_;
  content::WebUI* const web_ui_;
  ui::MojoBubbleWebUIController* const webui_controller_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PANEL_HANDLER_WALLET_PANEL_HANDLER_H_
