// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_INTERNALS_UI_H_

#include <string_view>

#include "brave/components/local_ai/core/on_device_speech_recognition.mojom-forward.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace local_ai {

// WebUI controller for
// brave://on-device-speech-recognition-internals/.
// Test harness for verifying the
// OnDeviceSpeechRecognitionService pipeline.
class OnDeviceSpeechRecognitionInternalsUI : public content::WebUIController {
 public:
  OnDeviceSpeechRecognitionInternalsUI(content::WebUI* web_ui,
                                       std::string_view host);
  OnDeviceSpeechRecognitionInternalsUI(
      const OnDeviceSpeechRecognitionInternalsUI&) = delete;
  OnDeviceSpeechRecognitionInternalsUI& operator=(
      const OnDeviceSpeechRecognitionInternalsUI&) = delete;
  ~OnDeviceSpeechRecognitionInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_INTERNALS_UI_H_
