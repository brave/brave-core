/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/polkadot/polkadot_ui.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_ui.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/polkadot_bridge/resources/grit/polkadot_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/resources/grit/webui_resources.h"

namespace polkadot {

UntrustedPolkadotUI::UntrustedPolkadotUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui),
      ui::EnableMojoWebUI(web_ui, false, false) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedPolkadotURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_POLKADOT_BRIDGE_HTML);
  untrusted_source->AddResourcePaths(kPolkadotBridgeGenerated);
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  // The frame decodes only what arrives over mojo. Denying it a network of its
  // own is what keeps the endpoint from being able to influence what the user
  // is told they are signing.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'none';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'wasm-unsafe-eval';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'unsafe-inline';"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
}

UntrustedPolkadotUI::~UntrustedPolkadotUI() = default;

void UntrustedPolkadotUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::PolkadotBridgeUIHandler>
        receiver) {
  receiver_.Bind(std::move(receiver));
}

std::unique_ptr<content::WebUIController>
UntrustedPolkadotUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                 const GURL& url) {
  return std::make_unique<UntrustedPolkadotUI>(web_ui);
}

UntrustedPolkadotUIConfig::UntrustedPolkadotUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedPolkadotHost) {}

void UntrustedPolkadotUI::BindPolkadotBridge(
    mojo::PendingRemote<brave_wallet::mojom::PolkadotBridge> bridge) {
  content::RenderFrameHost* rfh =
      web_ui()->GetWebContents()->GetPrimaryMainFrame();
  if (!rfh) {
    return;
  }

  CHECK(rfh->GetWebUI());
  content::WebUIController* controller = rfh->GetWebUI()->GetController();

  if (auto* panel = controller->GetAs<WalletPanelUI>()) {
    panel->BindPolkadotBridge(std::move(bridge));
    return;
  }
}

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedPolkadotUI)

}  // namespace polkadot
