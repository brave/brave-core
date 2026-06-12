// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_ORT_WORKER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_ORT_WORKER_UI_H_

#include <memory>

#include "brave/components/local_ai/core/local_ai.mojom-forward.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace local_ai {

// WebUI controller for the
// chrome-untrusted://on-device-speech-recognition-ort-worker/ page that
// hosts the onnxruntime-web (ORT-Web) Nemotron 0.6B backend. The page
// is cross-origin isolated (COOP/COEP) so ORT-Web's multi-threaded WASM
// backend (SharedArrayBuffer) can run; it is kept on its own origin so
// that isolation is contained to this worker.
//
// The ONNX model is delivered over mojo. The onnxruntime-web runtime
// distribution files are bundled into this page's pak (see
// components/local_ai/resources/speech_worker_ort) and served under /ort-dist/.
class UntrustedOnDeviceSpeechRecognitionOrtWorkerUI
    : public ui::MojoWebUIController {
 public:
  explicit UntrustedOnDeviceSpeechRecognitionOrtWorkerUI(
      content::WebUI* web_ui);
  UntrustedOnDeviceSpeechRecognitionOrtWorkerUI(
      const UntrustedOnDeviceSpeechRecognitionOrtWorkerUI&) = delete;
  UntrustedOnDeviceSpeechRecognitionOrtWorkerUI& operator=(
      const UntrustedOnDeviceSpeechRecognitionOrtWorkerUI&) = delete;
  ~UntrustedOnDeviceSpeechRecognitionOrtWorkerUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

// Registers the
// chrome-untrusted://on-device-speech-recognition-ort-worker/ URL and
// creates UntrustedOnDeviceSpeechRecognitionOrtWorkerUI instances for it.
class UntrustedOnDeviceSpeechRecognitionOrtWorkerUIConfig
    : public content::WebUIConfig {
 public:
  UntrustedOnDeviceSpeechRecognitionOrtWorkerUIConfig();
  ~UntrustedOnDeviceSpeechRecognitionOrtWorkerUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_ORT_WORKER_UI_H_
