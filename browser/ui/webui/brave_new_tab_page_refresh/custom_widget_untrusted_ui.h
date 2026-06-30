// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_CUSTOM_WIDGET_UNTRUSTED_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_CUSTOM_WIDGET_UNTRUSTED_UI_H_

#include <memory>

#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace content {
class BrowserContext;
class WebUI;
class WebUIController;
}  // namespace content

class GURL;

namespace brave_new_tab_page_refresh {

// Config for the chrome-untrusted host that renders user-added, AI-generated
// custom NTP widgets. Only enabled behind the kBraveNtpCustomWidgets flag.
class CustomWidgetUntrustedUIConfig : public content::WebUIConfig {
 public:
  CustomWidgetUntrustedUIConfig();
  ~CustomWidgetUntrustedUIConfig() override;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

// Untrusted WebUI that receives a self-contained HTML widget document from the
// embedding New Tab Page (via postMessage) and renders it inside a sandboxed
// child iframe. It is bound to zero mojom interfaces, so widget code runs with
// an opaque origin, no browser privileges, and only the network access allowed
// by this page's connect-src CSP allow-list.
class CustomWidgetUntrustedUI : public ui::UntrustedWebUIController {
 public:
  explicit CustomWidgetUntrustedUI(content::WebUI* web_ui);
  ~CustomWidgetUntrustedUI() override;

  CustomWidgetUntrustedUI(const CustomWidgetUntrustedUI&) = delete;
  CustomWidgetUntrustedUI& operator=(const CustomWidgetUntrustedUI&) = delete;
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_CUSTOM_WIDGET_UNTRUSTED_UI_H_
