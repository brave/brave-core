// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/local_ai/local_ai_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/local_ai_generated.h"
#include "brave/components/local_ai/resources/grit/local_ai_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

UntrustedLocalAIUI::UntrustedLocalAIUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedLocalAIURL);

  source->AddResourcePaths(kCandleEmbeddingModuleGenerated);
  webui::SetupWebUIDataSource(source, kLocalAiGenerated, IDR_LOCAL_AI_HTML);

  // Set up CSP to allow WASM execution and Mojo JS from resources
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome-untrusted://resources 'self' 'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc, "style-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc, "font-src 'self' data:;");
}

UntrustedLocalAIUI::~UntrustedLocalAIUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedLocalAIUI)

void UntrustedLocalAIUI::BindInterface(
    mojo::PendingReceiver<mojom::LocalAIService> receiver) {
  passage_embeddings::BravePassageEmbeddingsService::BindForWebContents(
      web_ui()->GetWebContents(), std::move(receiver));
}

///////////////////////////////////////////////////////////////////////////////

UntrustedLocalAIUIConfig::UntrustedLocalAIUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kUntrustedLocalAIHost) {}

std::unique_ptr<content::WebUIController>
UntrustedLocalAIUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                const GURL& url) {
  return std::make_unique<UntrustedLocalAIUI>(web_ui);
}

}  // namespace local_ai
