// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/candle_wasm/candle_embedding_gemma_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/local_ai/local_ai_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_bridge_generated.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_bridge_generated_map.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_generated.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_generated_map.h"
#include "chrome/browser/profiles/profile.h"
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
      kUntrustedOnDeviceModelWorkerURL);

  source->AddResourcePaths(kCandleEmbeddingGemmaGenerated);
  webui::SetupWebUIDataSource(source, kCandleEmbeddingGemmaBridgeGenerated,
                              IDR_CANDLE_EMBEDDING_GEMMA_BRIDGE_HTML);

  // Set up CSP to allow WASM execution and Mojo JS from resources
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome-untrusted://resources 'self' 'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc, "style-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc, "font-src 'self' data:;");
}

UntrustedCandleEmbeddingGemmaUI::~UntrustedCandleEmbeddingGemmaUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedCandleEmbeddingGemmaUI)

void UntrustedCandleEmbeddingGemmaUI::BindInterface(
    mojo::PendingReceiver<mojom::LocalAIService> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  LocalAIServiceFactory::BindForProfile(profile, std::move(receiver));
}

///////////////////////////////////////////////////////////////////////////////

UntrustedCandleEmbeddingGemmaUIConfig::UntrustedCandleEmbeddingGemmaUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kUntrustedOnDeviceModelWorkerHost) {}

std::unique_ptr<content::WebUIController>
UntrustedCandleEmbeddingGemmaUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<UntrustedCandleEmbeddingGemmaUI>(web_ui);
}

}  // namespace local_ai
