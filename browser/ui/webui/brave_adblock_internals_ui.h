// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_INTERNALS_UI_H_

#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"

class BraveAdblockInternalsUI;

class BraveAdblockInternalsUIConfig
    : public content::DefaultWebUIConfig<BraveAdblockInternalsUI> {
 public:
  BraveAdblockInternalsUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kAdblockInternalsHost) {}
};

// The WebUI for brave://adblock-internals
class BraveAdblockInternalsUI : public content::WebUIController {
 public:
  explicit BraveAdblockInternalsUI(content::WebUI* web_ui);

  BraveAdblockInternalsUI(const BraveAdblockInternalsUI&) = delete;
  BraveAdblockInternalsUI& operator=(const BraveAdblockInternalsUI&) = delete;

  ~BraveAdblockInternalsUI() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_INTERNALS_UI_H_
