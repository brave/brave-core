// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_ON_DEVICE_MODEL_WORKER_WEBUI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_ON_DEVICE_MODEL_WORKER_WEBUI_H_


#include <memory>
#include <string>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"
#include "content/public/browser/webui_config.h"

namespace content {
class BrowserContext;
}

class Profile;

class UntrustedOnDeviceModelWorkerWebUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedOnDeviceModelWorkerWebUI(content::WebUI* web_ui);
  UntrustedOnDeviceModelWorkerWebUI(const UntrustedOnDeviceModelWorkerWebUI&) = delete;
  UntrustedOnDeviceModelWorkerWebUI& operator=(const UntrustedOnDeviceModelWorkerWebUI&) = delete;
  ~UntrustedOnDeviceModelWorkerWebUI() override;

  void BindInterface(mojo::PendingReceiver<ai_chat::mojom::Service> receiver);

  static constexpr std::string GetWebUIName() { return "OnDeviceModelWorker"; }

 private:
  raw_ptr<Profile> profile_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedOnDeviceModelWorkerWebUIConfig : public content::WebUIConfig {
 public:
  UntrustedOnDeviceModelWorkerWebUIConfig();
  ~UntrustedOnDeviceModelWorkerWebUIConfig() override = default;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};



#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_ON_DEVICE_MODEL_WORKER_WEBUI_H_
