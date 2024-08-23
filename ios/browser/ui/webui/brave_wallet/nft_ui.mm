// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/nft_ui.h"

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/nft_display/resources/grit/nft_display_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/components/webui/web_ui_url_constants.h"
#import "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/resources/grit/webui_resources.h"

namespace {

web::WebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    const webui::ResourcePath* resource_map,
    std::size_t resource_map_size,
    int html_resource_id) {
  web::WebUIIOSDataSource* source = web::WebUIIOSDataSource::Create(name);
  web::WebUIIOSDataSource::Add(ChromeBrowserState::FromWebUIIOS(web_ui),
                               source);
  source->UseStringsJs();

  // Add required resources.
  source->AddResourcePaths(base::make_span(resource_map, resource_map_size));
  source->SetDefaultResource(html_resource_id);
  return source;
}

}  // namespace

// kUntrustedNftURL
UntrustedNftUI::UntrustedNftUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()) {
  web::WebUIIOSDataSource* untrusted_source = CreateAndAddWebUIDataSource(
      web_ui, url.host(), kNftDisplayGenerated, kNftDisplayGeneratedSize,
      IDR_BRAVE_WALLET_NFT_DISPLAY_HTML);

  for (const auto& str : brave_wallet::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    untrusted_source->AddString(str.name, l10n_str);
  }

  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      base::StrCat({"script-src self", kChromeUIUntrustedScheme,
                    url::kStandardSchemeSeparator, "resources;"}));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'self' 'unsafe-inline';"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' data:;"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  untrusted_source->AddString("braveWalletMarketUiBridgeUrl",
                              kUntrustedMarketURL);
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' https: data:;"));
}

UntrustedNftUI::~UntrustedNftUI() = default;

bool UntrustedNftUI::OverrideHandleWebUIIOSMessage(const GURL& source_url,
                                                   std::string_view message) {
  return false;
}
