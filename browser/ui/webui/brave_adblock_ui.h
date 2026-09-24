/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_

#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"

class BraveAdblockUI;

class BraveAdblockUIConfig
    : public content::DefaultWebUIConfig<BraveAdblockUI> {
 public:
  BraveAdblockUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kAdblockHost) {}
};

class BraveAdblockUI : public content::WebUIController {
 public:
  explicit BraveAdblockUI(content::WebUI* web_ui);
  ~BraveAdblockUI() override;
  BraveAdblockUI(const BraveAdblockUI&) = delete;
  BraveAdblockUI& operator=(const BraveAdblockUI&) = delete;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_
