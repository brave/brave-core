/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/ledger/ledger_ui.h"

#include <string>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ledger_bridge/resources/grit/ledger_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/resources/grit/webui_resources.h"

namespace ledger {

UntrustedLedgerUI::UntrustedLedgerUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedLedgerURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_LEDGER_BRIDGE_HTML);
  untrusted_source->AddResourcePaths(base::span(kLedgerBridgeGenerated));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'unsafe-inline';"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletLedgerBridgeUrl",
                              kUntrustedLedgerURL);
}

UntrustedLedgerUI::~UntrustedLedgerUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedLedgerUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                               const GURL& url) {
  return std::make_unique<UntrustedLedgerUI>(web_ui);
}

UntrustedLedgerUIConfig::UntrustedLedgerUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedLedgerHost) {}

}  // namespace ledger
