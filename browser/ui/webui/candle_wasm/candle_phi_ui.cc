// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/candle_wasm/candle_phi_ui.h"

#include <utility>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/browser/candle_service.h"
#include "brave/components/local_ai/resources/grit/candle_phi_bridge_generated.h"
#include "brave/components/local_ai/resources/grit/candle_phi_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

UntrustedCandlePhiUI::UntrustedCandlePhiUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(),
      kUntrustedCandlePhiWasmURL);

  // Set default resource and add generated paths
  source->SetDefaultResource(IDR_CANDLE_PHI_BRIDGE_HTML);
  source->AddResourcePaths(kCandlePhiBridgeGenerated);
  // Allow embedding in the local-ai-internals page
  source->AddFrameAncestor(GURL(kLocalAIInternalsURL));

  // Setup WebUI data source with generated resources
  webui::SetupWebUIDataSource(source, kCandlePhiBridgeGenerated,
                              IDR_CANDLE_PHI_BRIDGE_HTML);

  // Set up CSP to allow WASM execution
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src chrome://resources "
                  "chrome-untrusted://resources "
                  "'self' 'wasm-unsafe-eval';"));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'self' 'unsafe-inline';"));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src chrome://resources chrome-untrusted://resources 'self' "
      "'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' data:;"));
}

UntrustedCandlePhiUI::~UntrustedCandlePhiUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedCandlePhiUI)

void UntrustedCandlePhiUI::BindInterface(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  CandleService::GetInstance()->BindReceiver(std::move(receiver));
}

///////////////////////////////////////////////////////////////////////////////

UntrustedCandlePhiUIConfig::UntrustedCandlePhiUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kUntrustedCandlePhiWasmHost) {}

std::unique_ptr<content::WebUIController>
UntrustedCandlePhiUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                  const GURL& url) {
  return std::make_unique<UntrustedCandlePhiUI>(web_ui);
}

}  // namespace local_ai
