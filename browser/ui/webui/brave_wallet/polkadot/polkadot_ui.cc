/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/polkadot/polkadot_ui.h"

#include <string>

#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/polkadot_bridge/resources/grit/polkadot_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/resources/grit/webui_resources.h"

namespace polkadot {

UntrustedPolkadotUI::UntrustedPolkadotUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedPolkadotURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_POLKADOT_BRIDGE_HTML);
  untrusted_source->AddResourcePaths(kPolkadotBridgeGenerated);
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src 'self' https://polkadot-asset-hub-rpc.polkadot.io;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src chrome://resources/js/ 'self' ") + ";");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'unsafe-inline';"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
}

UntrustedPolkadotUI::~UntrustedPolkadotUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedPolkadotUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                 const GURL& url) {
  return std::make_unique<UntrustedPolkadotUI>(web_ui);
}

UntrustedPolkadotUIConfig::UntrustedPolkadotUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedPolkadotHost) {}

}  // namespace polkadot
