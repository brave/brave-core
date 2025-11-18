// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/line_chart_ui.h"

#include <string>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/line_chart_display/resources/grit/line_chart_display_generated_map.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/grit/brave_components_resources.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/webui/webui_util.h"

namespace line_chart {

UntrustedLineChartUI::UntrustedLineChartUI(web::WebUIIOS* web_ui,
                                           const GURL& url)
    : web::WebUIIOSController(web_ui, url.GetHost()) {
  BraveWebUIIOSDataSource* untrusted_source =
      brave::CreateAndAddWebUIDataSource(
          web_ui, url.host(), kLineChartDisplayGenerated,
          IDR_BRAVE_WALLET_LINE_CHART_DISPLAY_HTML);

  untrusted_source->AddLocalizedStrings(brave_wallet::kLocalizedStrings);

  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src 'self' chrome-untrusted://resources "
                  "chrome-untrusted://brave-resources;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string(
          "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' data: chrome-untrusted://resources;"));
  untrusted_source->AddString("braveWalletLineChartBridgeUrl",
                              kUntrustedLineChartURL);
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' data:;"));
}

UntrustedLineChartUI::~UntrustedLineChartUI() = default;

}  // namespace line_chart
