// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/local_ai/on_device_speech_recognition_worker_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/speech/on_device_speech_recognition_controller.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/resources/grit/candle_nemotron_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_nemotron_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/candle_nemotron_q8_0_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_nemotron_q8_0_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_600m_f16_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_600m_f16_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_600m_q4_k_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_600m_q4_k_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_600m_q8_0_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_600m_q8_0_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_parakeet_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_worker_generated.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_worker_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

UntrustedOnDeviceSpeechRecognitionWorkerUI::
    UntrustedOnDeviceSpeechRecognitionWorkerUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(),
      kOnDeviceSpeechRecognitionWorkerURL);

  source->AddResourcePaths(kCandleParakeetModuleGenerated);
  source->AddResourcePaths(kCandleNemotronModuleGenerated);
  source->AddResourcePaths(kCandleNemotronQ80ModuleGenerated);
  source->AddResourcePaths(kCandleParakeet600mF16ModuleGenerated);
  source->AddResourcePaths(kCandleParakeet600mQ4KModuleGenerated);
  source->AddResourcePaths(kCandleParakeet600mQ80ModuleGenerated);
  webui::SetupWebUIDataSource(source, kOnDeviceSpeechRecognitionWorkerGenerated,
                              IDR_ON_DEVICE_SPEECH_RECOGNITION_WORKER_HTML);

  // WASM + mojo JS bindings require these CSP relaxations.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome-untrusted://resources 'self' 'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc, "style-src 'self';");
}

UntrustedOnDeviceSpeechRecognitionWorkerUI::
    ~UntrustedOnDeviceSpeechRecognitionWorkerUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedOnDeviceSpeechRecognitionWorkerUI)

void UntrustedOnDeviceSpeechRecognitionWorkerUI::BindInterface(
    mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver) {
  speech::OnDeviceSpeechRecognitionController::Get()->BindForWebContents(
      std::move(receiver));
}

///////////////////////////////////////////////////////////////////////////////

UntrustedOnDeviceSpeechRecognitionWorkerUIConfig::
    UntrustedOnDeviceSpeechRecognitionWorkerUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kOnDeviceSpeechRecognitionWorkerHost) {}

std::unique_ptr<content::WebUIController>
UntrustedOnDeviceSpeechRecognitionWorkerUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<UntrustedOnDeviceSpeechRecognitionWorkerUI>(web_ui);
}

}  // namespace local_ai
