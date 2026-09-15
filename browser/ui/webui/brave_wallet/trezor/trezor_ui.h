/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_TREZOR_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_TREZOR_UI_H_

#include <memory>

#include "brave/components/brave_wallet/common/trezor_bridge.mojom.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace trezor {

// Untrusted WebUI (`chrome-untrusted://trezor-bridge`) that hosts the
// `@trezor/connect-web` library. The frontend (`trezor.ts`) supports two
// transports side-by-side, selected at runtime via a loadTimeData flag: the
// legacy `postMessage` protocol (handled entirely in JS, no browser-side
// involvement) and a Mojom-based `TrezorBridge` interface, brokered here.
class UntrustedTrezorUI : public ui::UntrustedWebUIController,
                          public ui::EnableMojoWebUI,
                          public brave_wallet::mojom::TrezorBridgeUIHandler {
 public:
  explicit UntrustedTrezorUI(content::WebUI* web_ui);
  UntrustedTrezorUI(const UntrustedTrezorUI&) = delete;
  UntrustedTrezorUI& operator=(const UntrustedTrezorUI&) = delete;
  ~UntrustedTrezorUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_wallet::mojom::TrezorBridgeUIHandler>
          receiver);

 private:
  // mojom::TrezorBridgeUIHandler:
  void BindTrezorBridge(
      mojo::PendingRemote<brave_wallet::mojom::TrezorBridge> bridge) override;

  mojo::Receiver<brave_wallet::mojom::TrezorBridgeUIHandler> receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedTrezorUIConfig : public content::WebUIConfig {
 public:
  UntrustedTrezorUIConfig();
  ~UntrustedTrezorUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace trezor

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_TREZOR_UI_H_
