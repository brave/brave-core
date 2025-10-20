// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/candle_wasm/candle_embedding_gemma_ui.h"

#include <utility>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/browser/candle_service.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_bridge_generated.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

UntrustedCandleEmbeddingGemmaUI::UntrustedCandleEmbeddingGemmaUI(
    content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(),
      kUntrustedCandleEmbeddingGemmaWasmURL);

  // Set default resource and add generated paths
  source->SetDefaultResource(IDR_CANDLE_EMBEDDING_GEMMA_BRIDGE_HTML);
  source->AddResourcePaths(kCandleEmbeddingGemmaBridgeGenerated);
  // Allow embedding in the local-ai-internals page
  source->AddFrameAncestor(GURL(kLocalAIInternalsURL));

  // Setup WebUI data source with generated resources
  webui::SetupWebUIDataSource(source, kCandleEmbeddingGemmaBridgeGenerated,
                              IDR_CANDLE_EMBEDDING_GEMMA_BRIDGE_HTML);

  // Set up CSP to allow WASM execution
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src chrome://resources chrome-untrusted://resources "
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

UntrustedCandleEmbeddingGemmaUI::~UntrustedCandleEmbeddingGemmaUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedCandleEmbeddingGemmaUI)

void UntrustedCandleEmbeddingGemmaUI::BindInterface(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  CandleService::GetInstance()->BindReceiver(std::move(receiver));
}

///////////////////////////////////////////////////////////////////////////////

UntrustedCandleEmbeddingGemmaUIConfig::UntrustedCandleEmbeddingGemmaUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kUntrustedCandleEmbeddingGemmaWasmHost) {}

std::unique_ptr<content::WebUIController>
UntrustedCandleEmbeddingGemmaUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<UntrustedCandleEmbeddingGemmaUI>(web_ui);
}

}  // namespace local_ai
