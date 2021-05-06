// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PAGE_HANDLER_WALLET_PANEL_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PAGE_HANDLER_WALLET_PANEL_PAGE_HANDLER_H_

#include "brave/components/brave_wallet_ui/wallet_panel.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace content {
class WebUI;
}

class WalletPanelUI;

class WalletPanelPageHandler : public wallet_panel::mojom::PageHandler,
                               public content::WebContentsObserver {
 public:
  WalletPanelPageHandler(
      mojo::PendingReceiver<wallet_panel::mojom::PageHandler> receiver,
      mojo::PendingRemote<wallet_panel::mojom::Page> page,
      content::WebUI* web_ui,
      ui::MojoBubbleWebUIController* webui_controller);
  WalletPanelPageHandler(const WalletPanelPageHandler&) = delete;
  WalletPanelPageHandler& operator=(const WalletPanelPageHandler&) = delete;
  ~WalletPanelPageHandler() override;

  // content::WebContentsObserver:
  void OnVisibilityChanged(content::Visibility visibility) override;

  // wallet_panel::mojom::PageHandler:
  void ShowUI() override;
  void CloseUI() override;

 private:
  bool webui_hidden_ = false;
  mojo::Receiver<wallet_panel::mojom::PageHandler> receiver_;
  mojo::Remote<wallet_panel::mojom::Page> page_;
  content::WebUI* const web_ui_;
  ui::MojoBubbleWebUIController* const webui_controller_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PAGE_HANDLER_WALLET_PANEL_PAGE_HANDLER_H_
