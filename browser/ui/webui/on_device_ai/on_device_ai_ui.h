// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_ON_DEVICE_AI_ON_DEVICE_AI_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_ON_DEVICE_AI_ON_DEVICE_AI_UI_H_

#include "brave/components/on_device_ai/core/on_device_ai.mojom-forward.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace on_device_ai {

// WebUI controller for the chrome-untrusted://on-device-ai/
// page that hosts the on-device embedding model compiled to
// WebAssembly. Runs in an isolated (untrusted) renderer process since
// it processes arbitrary page text for embedding generation.
class UntrustedOnDeviceAIUI : public ui::MojoWebUIController {
 public:
  explicit UntrustedOnDeviceAIUI(content::WebUI* web_ui);
  UntrustedOnDeviceAIUI(const UntrustedOnDeviceAIUI&) = delete;
  UntrustedOnDeviceAIUI& operator=(const UntrustedOnDeviceAIUI&) = delete;
  ~UntrustedOnDeviceAIUI() override;

  void BindInterface(mojo::PendingReceiver<mojom::OnDeviceAIService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

// Registers the chrome-untrusted://on-device-ai/ URL and
// creates UntrustedOnDeviceAIUI instances for it.
class UntrustedOnDeviceAIUIConfig : public content::WebUIConfig {
 public:
  UntrustedOnDeviceAIUIConfig();
  ~UntrustedOnDeviceAIUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace on_device_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_ON_DEVICE_AI_ON_DEVICE_AI_UI_H_
