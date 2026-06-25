/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_SNAP_HOST_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_SNAP_HOST_UI_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

// WalletSnapHostUI: Chrome WebUI controller for chrome://wallet-snap-host/.
//
// This page is a minimal, trusted (chrome://) host whose sole job is to:
//   1. Load the snap bridge JavaScript bundle.
//   2. Create chrome-untrusted://snap-executor iframes for each snap.
//   3. Bind mojom::SnapBridge back to the browser process via
//      WalletSnapHostHandlerFactory::CreateSnapHostHandler.
//
// It has no wallet UI of its own. It is used when
// features::kBraveWalletSnapsBackground is enabled and a hidden (or debug
// foreground) WebContents serves as the snap execution environment.
class WalletSnapHostUI
    : public ui::MojoWebUIController,
      public brave_wallet::mojom::WalletSnapHostHandlerFactory {
 public:
  explicit WalletSnapHostUI(content::WebUI* web_ui);
  WalletSnapHostUI(const WalletSnapHostUI&) = delete;
  WalletSnapHostUI& operator=(const WalletSnapHostUI&) = delete;
  ~WalletSnapHostUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_wallet::mojom::WalletSnapHostHandlerFactory>
          receiver);

 private:
  // brave_wallet::mojom::WalletSnapHostHandlerFactory:
  void CreateSnapHostHandler(
      mojo::PendingRemote<brave_wallet::mojom::SnapBridge> snap_bridge,
      mojo::PendingReceiver<brave_wallet::mojom::SnapRequestHandler>
          snap_request_handler,
      mojo::PendingReceiver<brave_wallet::mojom::SnapsService> snaps_service)
      override;

  mojo::Receiver<brave_wallet::mojom::WalletSnapHostHandlerFactory>
      factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class WalletSnapHostUIConfig
    : public content::DefaultWebUIConfig<WalletSnapHostUI> {
 public:
  WalletSnapHostUIConfig();
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_SNAP_HOST_UI_H_
