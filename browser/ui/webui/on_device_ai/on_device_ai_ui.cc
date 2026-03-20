// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/on_device_ai/on_device_ai_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/on_device_ai/on_device_ai_service_factory.h"
#include "brave/components/on_device_ai/core/on_device_ai.mojom.h"
#include "brave/components/on_device_ai/core/url_constants.h"
#include "brave/components/on_device_ai/resources/grit/candle_embedding_module_generated.h"
#include "brave/components/on_device_ai/resources/grit/candle_embedding_module_generated_map.h"
#include "brave/components/on_device_ai/resources/grit/on_device_ai_generated.h"
#include "brave/components/on_device_ai/resources/grit/on_device_ai_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace on_device_ai {

UntrustedOnDeviceAIUI::UntrustedOnDeviceAIUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedOnDeviceAIURL);

  source->AddResourcePaths(kCandleEmbeddingModuleGenerated);
  webui::SetupWebUIDataSource(source, kOnDeviceAiGenerated,
                              IDR_ON_DEVICE_AI_HTML);

  // Set up CSP to allow WASM execution and Mojo JS from resources
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome-untrusted://resources 'self' 'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc, "style-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc, "font-src 'self' data:;");
}

UntrustedOnDeviceAIUI::~UntrustedOnDeviceAIUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedOnDeviceAIUI)

void UntrustedOnDeviceAIUI::BindInterface(
    mojo::PendingReceiver<mojom::OnDeviceAIService> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  OnDeviceAIServiceFactory::BindForProfile(profile, std::move(receiver));
}

///////////////////////////////////////////////////////////////////////////////

UntrustedOnDeviceAIUIConfig::UntrustedOnDeviceAIUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kUntrustedOnDeviceAIHost) {}

std::unique_ptr<content::WebUIController>
UntrustedOnDeviceAIUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                   const GURL& url) {
  return std::make_unique<UntrustedOnDeviceAIUI>(web_ui);
}

}  // namespace on_device_ai
