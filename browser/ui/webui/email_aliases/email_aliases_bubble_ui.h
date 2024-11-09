// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_BUBBLE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_BUBBLE_UI_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/common/url_constants.h"

namespace email_aliases {

class EmailAliasesBubbleUI : public content::WebUIController {
 public:
  explicit EmailAliasesBubbleUI(content::WebUI* web_ui);
  ~EmailAliasesBubbleUI() override;

  EmailAliasesBubbleUI(const EmailAliasesBubbleUI&) = delete;
  EmailAliasesBubbleUI& operator=(const EmailAliasesBubbleUI&) = delete;

  static std::string GetWebUIName();

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class EmailAliasesBubbleUIConfig
    : public DefaultTopChromeWebUIConfig<EmailAliasesBubbleUI> {
 public:
  EmailAliasesBubbleUIConfig()
      : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                    kEmailAliasesBubbleHost) {}

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

}  // namespace email_aliases

#endif  // BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_BUBBLE_UI_H_
