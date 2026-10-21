// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WRAPPED_BRAVE_WRAPPED_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WRAPPED_BRAVE_WRAPPED_UI_H_

#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"

namespace content {
class BrowserContext;
class WebUI;
}  // namespace content

// brave://wrapped shows a shareable summary of the profile's lifetime Shields
// stats. Stats are a snapshot passed via loadTimeData; no mojo needed.
class BraveWrappedUI : public content::WebUIController {
 public:
  explicit BraveWrappedUI(content::WebUI* web_ui);
  ~BraveWrappedUI() override;

  BraveWrappedUI(const BraveWrappedUI&) = delete;
  BraveWrappedUI& operator=(const BraveWrappedUI&) = delete;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveWrappedUIConfig
    : public content::DefaultWebUIConfig<BraveWrappedUI> {
 public:
  BraveWrappedUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBraveWrappedHost) {}

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WRAPPED_BRAVE_WRAPPED_UI_H_
