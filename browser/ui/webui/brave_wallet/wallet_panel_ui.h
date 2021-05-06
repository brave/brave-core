/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_UI_H_

#include <memory>

#include "base/macros.h"
#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_panel_page_handler.h"
#include "brave/components/brave_wallet_ui/wallet_panel.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

#include "ui/webui/mojo_bubble_web_ui_controller.h"

class WalletPanelUI : public ui::MojoBubbleWebUIController,
                      public wallet_panel::mojom::PageHandlerFactory {
 public:
  explicit WalletPanelUI(content::WebUI* web_ui);
  WalletPanelUI(const WalletPanelUI&) = delete;
  WalletPanelUI& operator=(const WalletPanelUI&) = delete;
  ~WalletPanelUI() override;

  // Instantiates the implementor of the mojom::PageHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<wallet_panel::mojom::PageHandlerFactory> receiver);

 private:
  // wallet_panel::mojom::PageHandlerFactory:
  void CreatePageHandler(mojo::PendingRemote<wallet_panel::mojom::Page> page,
                         mojo::PendingReceiver<wallet_panel::mojom::PageHandler>
                             receiver) override;

  std::unique_ptr<WalletPanelPageHandler> page_handler_;

  mojo::Receiver<wallet_panel::mojom::PageHandlerFactory>
      page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_UI_H_
