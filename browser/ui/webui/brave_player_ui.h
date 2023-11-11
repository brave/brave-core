/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_PLAYER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_PLAYER_UI_H_

#include <memory>

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

// For now we don't need communication between page and controller, so
// using simple content::WebUIController instead of ui::MojoWebUIController.
class BravePlayerUI : public content::WebUIController {
 public:
  explicit BravePlayerUI(content::WebUI* web_ui);
  ~BravePlayerUI() override;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedBravePlayerEmbedUIConfig : public content::WebUIConfig {
 public:
  UntrustedBravePlayerEmbedUIConfig();
  ~UntrustedBravePlayerEmbedUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_PLAYER_UI_H_
