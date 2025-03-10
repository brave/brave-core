// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_BUBBLE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_BUBBLE_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/common/url_constants.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class View;
}  // namespace views

namespace content {
class WebContents;
}  // namespace content

class BrowserWindowInterface;
class WebUIBubbleManager;

namespace email_aliases {

class EmailAliasesBubbleUI : public TopChromeWebUIController,
                             public views::WidgetObserver {
 public:
  explicit EmailAliasesBubbleUI(content::WebUI* web_ui);
  ~EmailAliasesBubbleUI() override;

  EmailAliasesBubbleUI(const EmailAliasesBubbleUI&) = delete;
  EmailAliasesBubbleUI& operator=(const EmailAliasesBubbleUI&) = delete;

  static void Show(BrowserWindowInterface* browser_window_interface,
                   views::View* anchor_view,
                   content::WebContents* web_contents,
                   uint64_t field_renderer_id);
  static void Close();
  static void FillField(const std::string& alias_address);
  static constexpr std::string GetWebUIName() { return "ShieldsPanel"; }

 private:
  static std::unique_ptr<WebUIBubbleManager> webui_bubble_manager_;
  static raw_ptr<content::WebContents> web_contents_;
  static uint64_t field_renderer_id_;
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
