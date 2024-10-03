/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_LIVE_NTT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_LIVE_NTT_UI_H_

#include <memory>

#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

class UntrustedLiveNTTUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedLiveNTTUI(content::WebUI* web_ui);
  UntrustedLiveNTTUI(const UntrustedLiveNTTUI&) = delete;
  UntrustedLiveNTTUI& operator=(const UntrustedLiveNTTUI&) = delete;
  ~UntrustedLiveNTTUI() override;
};

class UntrustedLiveNTTUIConfig : public content::WebUIConfig {
 public:
  UntrustedLiveNTTUIConfig();
  ~UntrustedLiveNTTUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_LIVE_NTT_UI_H_
