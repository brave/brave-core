// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_RECAP_BRAVE_RECAP_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_RECAP_BRAVE_RECAP_UI_H_

#include "brave/components/brave_recap/common/webui_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"

namespace content {
class BrowserContext;
class WebUI;
}  // namespace content

// brave://recap: a shareable summary of the profile's lifetime Shields stats.
class BraveRecapUI : public content::WebUIController {
 public:
  explicit BraveRecapUI(content::WebUI* web_ui);
  ~BraveRecapUI() override;

  BraveRecapUI(const BraveRecapUI&) = delete;
  BraveRecapUI& operator=(const BraveRecapUI&) = delete;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveRecapUIConfig : public content::DefaultWebUIConfig<BraveRecapUI> {
 public:
  BraveRecapUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBraveRecapHost) {}

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_RECAP_BRAVE_RECAP_UI_H_
