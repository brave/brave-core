/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_HELLO_WORLD_HELLO_WORLD_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_HELLO_WORLD_HELLO_WORLD_UI_H_

#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"

class HelloWorldUI;

namespace content {
class WebUI;
}

class HelloWorldUIConfig : public content::DefaultWebUIConfig<HelloWorldUI> {
 public:
  HelloWorldUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kChromeUIHelloWorldHost) {}
};

class HelloWorldUI : public content::WebUIController {
 public:
  explicit HelloWorldUI(content::WebUI* web_ui);
  ~HelloWorldUI() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_HELLO_WORLD_HELLO_WORLD_UI_H_
