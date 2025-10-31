/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_wallet/market_ui.h"

#include <string>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/market_display/resources/grit/market_display_generated_map.h"
#include "brave/ios/browser/ui/webui/untrusted_sanitized_image_source.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/grit/brave_components_resources.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/webui/resources/grit/webui_resources.h"
#include "ui/webui/webui_util.h"

namespace market {

UntrustedMarketUI::UntrustedMarketUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()) {
  BraveWebUIIOSDataSource* untrusted_source =
      brave::CreateAndAddWebUIDataSource(web_ui, url.host(),
                                         kMarketDisplayGenerated,
                                         IDR_BRAVE_WALLET_MARKET_DISPLAY_HTML);

  untrusted_source->AddLocalizedStrings(brave_wallet::kLocalizedStrings);

  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src 'self' chrome-untrusted://resources;"));

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'self' 'unsafe-inline';"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'self' 'unsafe-inline' "
                  "chrome-untrusted://resources chrome-untrusted://theme;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' data: chrome-untrusted://resources;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' https://assets.cgproxy.brave.com "
                  "chrome-untrusted://resources chrome-untrusted://image;"));

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

  auto* profile = ProfileIOS::FromWebUIIOS(web_ui);
  web::URLDataSourceIOS::Add(profile,
                             new UntrustedSanitizedImageSource(profile));
}

UntrustedMarketUI::~UntrustedMarketUI() = default;

}  // namespace market
