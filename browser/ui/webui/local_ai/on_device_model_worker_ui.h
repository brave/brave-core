// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_MODEL_WORKER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_MODEL_WORKER_UI_H_

#include "brave/components/local_ai/core/local_ai.mojom-forward.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace local_ai {

// WebUI controller for the chrome-untrusted://on-device-model-worker/
// page that hosts the on-device embedding model compiled to
// WebAssembly. Runs in an isolated (untrusted) renderer process since
// it processes arbitrary page text for embedding generation.
class UntrustedOnDeviceModelWorkerUI : public ui::MojoWebUIController {
 public:
  explicit UntrustedOnDeviceModelWorkerUI(content::WebUI* web_ui);
  UntrustedOnDeviceModelWorkerUI(const UntrustedOnDeviceModelWorkerUI&) =
      delete;
  UntrustedOnDeviceModelWorkerUI& operator=(
      const UntrustedOnDeviceModelWorkerUI&) = delete;
  ~UntrustedOnDeviceModelWorkerUI() override;

  void BindInterface(mojo::PendingReceiver<mojom::LocalAIService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

// Registers the chrome-untrusted://on-device-model-worker/ URL and
// creates UntrustedOnDeviceModelWorkerUI instances for it.
class UntrustedOnDeviceModelWorkerUIConfig : public content::WebUIConfig {
 public:
  UntrustedOnDeviceModelWorkerUIConfig();
  ~UntrustedOnDeviceModelWorkerUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_ON_DEVICE_MODEL_WORKER_UI_H_
