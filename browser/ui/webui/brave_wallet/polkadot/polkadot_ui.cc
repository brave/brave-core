/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/polkadot/polkadot_ui.h"

#include <string>
#include <utility>

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

  // Keep the @polkadot/types code air gapped.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'none';");

  // Some of the compiled Polkadot code can theoretically invoke wasm.
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

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedPolkadotUI)

}  // namespace polkadot
