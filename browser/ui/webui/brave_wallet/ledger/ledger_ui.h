/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_UI_H_

#include <memory>

#include "brave/components/brave_wallet/common/ledger_bridge.mojom-forward.h"
#include "brave/components/brave_wallet/common/ledger_bridge.mojom.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ledger {

// Untrusted WebUI (`chrome-untrusted://ledger-bridge`) that hosts the Ledger
// `@ledgerhq/*` JS library. The frontend (`ledger.ts`) supports two transports
// side-by-side, selected at runtime via a loadTimeData flag: the legacy
// `postMessage` protocol (handled entirely in JS, no browser-side involvement)
// and a Mojom-based `LedgerBridge` interface, brokered here.
class UntrustedLedgerUI : public ui::UntrustedWebUIController,
                          public ui::EnableMojoWebUI,
                          public brave_wallet::mojom::LedgerBridgeUIHandler {
 public:
  explicit UntrustedLedgerUI(content::WebUI* web_ui);
  UntrustedLedgerUI(const UntrustedLedgerUI&) = delete;
  UntrustedLedgerUI& operator=(const UntrustedLedgerUI&) = delete;
  ~UntrustedLedgerUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_wallet::mojom::LedgerBridgeUIHandler>
          receiver);

 private:
  // mojom::LedgerBridgeUIHandler:
  void BindLedgerBridge(
      mojo::PendingRemote<brave_wallet::mojom::LedgerBridge> bridge) override;

  mojo::Receiver<brave_wallet::mojom::LedgerBridgeUIHandler> receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedLedgerUIConfig : public content::WebUIConfig {
 public:
  UntrustedLedgerUIConfig();
  ~UntrustedLedgerUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace ledger

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_UI_H_
