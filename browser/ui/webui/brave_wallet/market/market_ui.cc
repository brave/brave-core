/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/market/market_ui.h"

#include <string>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/market_display/resources/grit/market_display_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/resources/grit/webui_generated_resources.h"

namespace market {

UntrustedMarketUI::UntrustedMarketUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source =
      content::WebUIDataSource::Create(kUntrustedMarketURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_MARKET_DISPLAY_HTML);
  untrusted_source->AddResourcePaths(
      base::make_span(kMarketDisplayGenerated, kMarketDisplayGeneratedSize));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));

  // TODO(nvonpentz) Determine CSP. Commented below was copied from trezor_ui.cc
  //
  // untrusted_source->OverrideContentSecurityPolicy(
  //     network::mojom::CSPDirectiveName::StyleSrc,
  //     std::string("style-src 'unsafe-inline';"));
  untrusted_source->AddResourcePath("load_time_data.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletMarketDisplayUrl",
                              kUntrustedMarketURL);
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' assets.cgproxy.brave.com data:;"));
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource::Add(browser_context, untrusted_source);
}

UntrustedMarketUI::~UntrustedMarketUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedMarketUIConfig::CreateWebUIController(content::WebUI* web_ui) {
  return std::make_unique<UntrustedMarketUI>(web_ui);
}

UntrustedMarketUIConfig::UntrustedMarketUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedMarketHost) {}

}  // namespace market
