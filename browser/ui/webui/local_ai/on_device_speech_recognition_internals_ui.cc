/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/local_ai/on_device_speech_recognition_internals_ui.h"

#include <utility>

#include "brave/browser/speech/on_device_speech_recognition_controller.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_internals_generated.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_internals_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"

namespace local_ai {

OnDeviceSpeechRecognitionInternalsUI::OnDeviceSpeechRecognitionInternalsUI(
    content::WebUI* web_ui,
    std::string_view host)
    : content::WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, host,
                              kOnDeviceSpeechRecognitionInternalsGenerated,
                              IDR_ON_DEVICE_SPEECH_RECOGNITION_INTERNALS_HTML);
}

OnDeviceSpeechRecognitionInternalsUI::~OnDeviceSpeechRecognitionInternalsUI() =
    default;

void OnDeviceSpeechRecognitionInternalsUI::BindInterface(
    mojo::PendingReceiver<mojom::AsrSession> receiver) {
  speech::OnDeviceSpeechRecognitionController::Get()->BindAsrSession(
      std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(OnDeviceSpeechRecognitionInternalsUI)

}  // namespace local_ai
