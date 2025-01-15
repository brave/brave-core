/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/market/market_ui.h"

#include <string>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/market_display/resources/grit/market_display_generated_map.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/resources/grit/webui_resources.h"

namespace market {

UntrustedMarketUI::UntrustedMarketUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedMarketURL);

  for (const auto& str : brave_wallet::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    untrusted_source->AddString(str.name, l10n_str);
  }
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_MARKET_DISPLAY_HTML);
  untrusted_source->AddResourcePaths(kMarketDisplayGenerated);
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  webui::SetupWebUIDataSource(untrusted_source, kMarketDisplayGenerated,
                              IDR_BRAVE_WALLET_MARKET_DISPLAY_HTML);

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src 'self' chrome-untrusted://resources;"));

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'self' 'unsafe-inline';"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string(
          "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' data: chrome-untrusted://resources;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' https://assets.cgproxy.brave.com "
                  "chrome-untrusted://resources;"));

  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletTrezorBridgeUrl",
                              kUntrustedTrezorURL);
  untrusted_source->AddString("braveWalletLedgerBridgeUrl",
                              kUntrustedLedgerURL);
  untrusted_source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  untrusted_source->AddString("braveWalletMarketUiBridgeUrl",
                              kUntrustedMarketURL);
}

UntrustedMarketUI::~UntrustedMarketUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedMarketUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                               const GURL& url) {
  return std::make_unique<UntrustedMarketUI>(web_ui);
}

UntrustedMarketUIConfig::UntrustedMarketUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedMarketHost) {}

}  // namespace market
