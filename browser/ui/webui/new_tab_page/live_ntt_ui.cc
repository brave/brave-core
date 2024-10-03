/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/new_tab_page/live_ntt_ui.h"

#include <string>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/live_ntt/resources/grit/live_ntt_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/resources/grit/webui_resources.h"
#include "content/public/common/url_constants.h"

UntrustedLiveNTTUI::UntrustedLiveNTTUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedLiveNTTURL);
  untrusted_source->SetDefaultResource(IDR_LIVE_NTT_HTML);
  untrusted_source->AddResourcePaths(
      base::make_span(kLiveNttGenerated, kLiveNttGeneratedSize));
  untrusted_source->AddFrameAncestor(GURL(kBraveNewTabPageURL));
  // untrusted_source->OverrideContentSecurityPolicy(
  //     network::mojom::CSPDirectiveName::StyleSrc,
  //     std::string("style-src 'unsafe-inline';"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' data:;"));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string(
          "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveLiveNttUrl",
                              kUntrustedLiveNTTURL);
}

UntrustedLiveNTTUI::~UntrustedLiveNTTUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedLiveNTTUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                               const GURL& url) {
  return std::make_unique<UntrustedLiveNTTUI>(web_ui);
}

UntrustedLiveNTTUIConfig::UntrustedLiveNTTUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedLiveNTTHost) {}
