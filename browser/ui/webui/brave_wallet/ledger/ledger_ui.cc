/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/ledger/ledger_ui.h"

#include <string>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ledger_bridge/resources/grit/ledger_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/resources/grit/webui_generated_resources.h"

namespace ledger {

UntrustedLedgerUI::UntrustedLedgerUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source =
      content::WebUIDataSource::Create(kUntrustedLedgerURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_LEDGER_BRIDGE_HTML);
  untrusted_source->AddResourcePaths(
      base::make_span(kLedgerBridgeGenerated, kLedgerBridgeGeneratedSize));
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
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource::Add(browser_context, untrusted_source);
}

UntrustedLedgerUI::~UntrustedLedgerUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedLedgerUIConfig::CreateWebUIController(content::WebUI* web_ui) {
  return std::make_unique<UntrustedLedgerUI>(web_ui);
}

UntrustedLedgerUIConfig::UntrustedLedgerUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedLedgerHost) {}

}  // namespace ledger
