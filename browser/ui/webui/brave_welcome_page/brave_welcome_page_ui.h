// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

// The Web UI controller for the Brave welcome page.
class BraveWelcomePageUI : public content::WebUIController {
 public:
  explicit BraveWelcomePageUI(content::WebUI* web_ui);
  ~BraveWelcomePageUI() override;

  BraveWelcomePageUI(const BraveWelcomePageUI&) = delete;
  BraveWelcomePageUI& operator=(const BraveWelcomePageUI&) = delete;
};

class BraveWelcomePageUIConfig
    : public content::DefaultWebUIConfig<BraveWelcomePageUI> {
 public:
  BraveWelcomePageUIConfig();

  // WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_UI_H_
