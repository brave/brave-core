// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_WORKER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_WORKER_UI_H_

#include <memory>

#include "brave/components/local_ai/core/on_device_speech_recognition.mojom-forward.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace local_ai {

// WebUI controller for the
// chrome-untrusted://on-device-speech-recognition-worker/
// page that hosts the speech recognition model compiled to
// WebAssembly. Runs in an isolated (untrusted) renderer.
class UntrustedOnDeviceSpeechRecognitionWorkerUI
    : public ui::MojoWebUIController {
 public:
  explicit UntrustedOnDeviceSpeechRecognitionWorkerUI(content::WebUI* web_ui);
  UntrustedOnDeviceSpeechRecognitionWorkerUI(
      const UntrustedOnDeviceSpeechRecognitionWorkerUI&) = delete;
  UntrustedOnDeviceSpeechRecognitionWorkerUI& operator=(
      const UntrustedOnDeviceSpeechRecognitionWorkerUI&) = delete;
  ~UntrustedOnDeviceSpeechRecognitionWorkerUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

// Registers the chrome-untrusted URL and creates UI
// instances.
class UntrustedOnDeviceSpeechRecognitionWorkerUIConfig
    : public content::WebUIConfig {
 public:
  UntrustedOnDeviceSpeechRecognitionWorkerUIConfig();
  ~UntrustedOnDeviceSpeechRecognitionWorkerUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_WORKER_UI_H_
