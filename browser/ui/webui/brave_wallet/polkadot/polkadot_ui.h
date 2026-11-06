/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_POLKADOT_POLKADOT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_POLKADOT_POLKADOT_UI_H_

#include <memory>

#include "brave/components/brave_wallet/common/polkadot_bridge.mojom.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace polkadot {

class UntrustedPolkadotUI
    : public ui::UntrustedWebUIController,
      public ui::EnableMojoWebUI,
      public brave_wallet::mojom::PolkadotBridgeUIHandler {
 public:
  explicit UntrustedPolkadotUI(content::WebUI* web_ui);
  UntrustedPolkadotUI(const UntrustedPolkadotUI&) = delete;
  UntrustedPolkadotUI& operator=(const UntrustedPolkadotUI&) = delete;
  ~UntrustedPolkadotUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_wallet::mojom::PolkadotBridgeUIHandler>
          receiver);

 private:
  // mojom::PolkadotBridgeUIHandler:
  void BindPolkadotBridge(
      mojo::PendingRemote<brave_wallet::mojom::PolkadotBridge> bridge) override;

  mojo::Receiver<brave_wallet::mojom::PolkadotBridgeUIHandler> receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedPolkadotUIConfig : public content::WebUIConfig {
 public:
  UntrustedPolkadotUIConfig();
  ~UntrustedPolkadotUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace polkadot

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_POLKADOT_POLKADOT_UI_H_
