/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/nft/nft_ui.h"

#include <string>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/nft_display/resources/grit/nft_display_generated_map.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/resources/grit/webui_generated_resources.h"

namespace nft {

UntrustedNftUI::UntrustedNftUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::Create(kUntrustedNftURL);

  for (const auto& str : brave_wallet::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    untrusted_source->AddString(str.name, l10n_str);
  }

  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_NFT_DISPLAY_HTML);
  untrusted_source->AddResourcePaths(
      base::make_span(kNftDisplayGenerated, kNftDisplayGeneratedSize));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  webui::SetupWebUIDataSource(
      untrusted_source,
      base::make_span(kNftDisplayGenerated, kNftDisplayGeneratedSize),
      IDR_BRAVE_WALLET_NFT_DISPLAY_HTML);
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src 'self' chrome-untrusted://resources;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'self' 'unsafe-inline';"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' data:;"));
  untrusted_source->AddResourcePath("load_time_data.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  untrusted_source->AddString("braveWalletTrezorBridgeUrl",
                              kUntrustedTrezorURL);
  untrusted_source->AddString("braveWalletLedgerBridgeUrl",
                              kUntrustedLedgerURL);
  untrusted_source->AddString("braveWalletMarketUiBridgeUrl",
                              kUntrustedMarketURL);
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' https: data:;"));
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource::Add(browser_context, untrusted_source);
}

UntrustedNftUI::~UntrustedNftUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedNftUIConfig::CreateWebUIController(content::WebUI* web_ui) {
  return std::make_unique<UntrustedNftUI>(web_ui);
}

UntrustedNftUIConfig::UntrustedNftUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedNftHost) {}

}  // namespace nft
