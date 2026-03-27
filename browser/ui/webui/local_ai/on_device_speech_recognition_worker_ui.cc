// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/local_ai/on_device_speech_recognition_worker_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/local_ai/on_device_speech_recognition_service_factory.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/resources/grit/candle_whisper_module_generated.h"
#include "brave/components/local_ai/resources/grit/candle_whisper_module_generated_map.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_worker_generated.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_worker_generated_map.h"
#include "chrome/browser/profiles/profile.h"
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
      kUntrustedOnDeviceSpeechRecognitionWorkerURL);

  source->AddResourcePaths(kCandleWhisperModuleGenerated);
  webui::SetupWebUIDataSource(source, kOnDeviceSpeechRecognitionWorkerGenerated,
                              IDR_ON_DEVICE_SPEECH_RECOGNITION_WORKER_HTML);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome-untrusted://resources "
      "'self' 'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc, "style-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc, "font-src 'self' data:;");
}

UntrustedOnDeviceSpeechRecognitionWorkerUI::
    ~UntrustedOnDeviceSpeechRecognitionWorkerUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedOnDeviceSpeechRecognitionWorkerUI)

void UntrustedOnDeviceSpeechRecognitionWorkerUI::BindInterface(
    mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  OnDeviceSpeechRecognitionServiceFactory::BindForProfile(profile,
                                                          std::move(receiver));
}

UntrustedOnDeviceSpeechRecognitionWorkerUIConfig::
    UntrustedOnDeviceSpeechRecognitionWorkerUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           "on-device-speech-recognition-worker") {}

std::unique_ptr<content::WebUIController>
UntrustedOnDeviceSpeechRecognitionWorkerUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<UntrustedOnDeviceSpeechRecognitionWorkerUI>(web_ui);
}

}  // namespace local_ai
