// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_BRAVE_NEWS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_BRAVE_NEWS_UI_H_

#include <string_view>

#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "chrome/browser/ui/webui/top_chrome/untrusted_top_chrome_web_ui_controller.h"

namespace content {
class BrowserContext;
class WebUI;
}  // namespace content

// Hosts the Brave News side panel page at chrome-untrusted://news/. The page
// content is currently a placeholder; the React frontend is wired up in a
// follow-up change.
class BraveNewsUI : public UntrustedTopChromeWebUIController {
 public:
  explicit BraveNewsUI(content::WebUI* web_ui);
  BraveNewsUI(const BraveNewsUI&) = delete;
  BraveNewsUI& operator=(const BraveNewsUI&) = delete;
  ~BraveNewsUI() override;

  static constexpr std::string_view GetWebUIName() { return "BraveNewsPanel"; }

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveNewsUIConfig : public DefaultTopChromeWebUIConfig<BraveNewsUI> {
 public:
  BraveNewsUIConfig();

  // TopChromeWebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_BRAVE_NEWS_UI_H_
