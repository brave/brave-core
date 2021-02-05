/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_

#include <string>

#include "brave/browser/ui/webui/basic_ui.h"

class BraveAdblockUI : public BasicUI {
 public:
  BraveAdblockUI(content::WebUI* web_ui, const std::string& host);
  ~BraveAdblockUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveAdblockUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_UI_H_
