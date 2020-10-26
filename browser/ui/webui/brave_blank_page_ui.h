/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_BLANK_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_BLANK_PAGE_UI_H_

#include <string>

#include "content/public/browser/web_ui_controller.h"

class BraveBlankPageUI : public content::WebUIController {
 public:
  BraveBlankPageUI(content::WebUI* web_ui,
                   const std::string& host);
  ~BraveBlankPageUI() override;

  BraveBlankPageUI(const BraveBlankPageUI&) = delete;
  BraveBlankPageUI& operator=(BraveBlankPageUI&) = delete;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_BLANK_PAGE_UI_H_
