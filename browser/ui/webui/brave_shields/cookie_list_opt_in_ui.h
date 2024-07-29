/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_UI_H_

#include <memory>
#include <string>

#include "brave/components/brave_shields/core/common/cookie_list_opt_in.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class BrowserContext;
}

class CookieListOptInUI
    : public TopChromeWebUIController,
      public brave_shields::mojom::CookieListOptInPageHandlerFactory {
 public:
  explicit CookieListOptInUI(content::WebUI* web_ui);
  ~CookieListOptInUI() override;

  CookieListOptInUI(const CookieListOptInUI&) = delete;
  CookieListOptInUI& operator=(const CookieListOptInUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<CookieListOptInPageHandlerFactory> reciever);

  static constexpr std::string GetWebUIName() {
    return "CookieListOptInBubblePanel";
  }

 private:
  // brave_shields::mojom::CookieListOptInPageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingReceiver<brave_shields::mojom::CookieListOptInPageHandler>
          receiver) override;

  std::unique_ptr<brave_shields::mojom::CookieListOptInPageHandler>
      page_handler_;
  mojo::Receiver<CookieListOptInPageHandlerFactory> page_factory_receiver_{
      this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class CookieListOptInUIConfig
    : public DefaultTopChromeWebUIConfig<CookieListOptInUI> {
 public:
  CookieListOptInUIConfig();

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_UI_H_
