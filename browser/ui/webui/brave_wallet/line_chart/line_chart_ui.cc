/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/browser/ui/webui/brave_wallet/line_chart/line_chart_ui.h"

#include <string>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/line_chart_display/resources/grit/line_chart_display_generated_map.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/resources/grit/webui_resources.h"

namespace line_chart {

UntrustedLineChartUI::UntrustedLineChartUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedLineChartURL);

  for (const auto& str : brave_wallet::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    untrusted_source->AddString(str.name, l10n_str);
  }

  untrusted_source->SetDefaultResource(
      IDR_BRAVE_WALLET_LINE_CHART_DISPLAY_HTML);
  untrusted_source->AddResourcePaths(base::make_span(
      kLineChartDisplayGenerated, kLineChartDisplayGeneratedSize));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  webui::SetupWebUIDataSource(untrusted_source,
                              base::make_span(kLineChartDisplayGenerated,
                                              kLineChartDisplayGeneratedSize),
                              IDR_BRAVE_WALLET_LINE_CHART_DISPLAY_HTML);
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

std::unique_ptr<content::WebUIController>
UntrustedLineChartUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                  const GURL& url) {
  return std::make_unique<UntrustedLineChartUI>(web_ui);
}

UntrustedLineChartUIConfig::UntrustedLineChartUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedLineChartHost) {}

}  // namespace line_chart
