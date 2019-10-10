/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_PAGE_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/basic_ui.h"

class BraveRewardsPageUI : public BasicUI {
 public:
  BraveRewardsPageUI(content::WebUI* web_ui, const std::string& host);
  ~BraveRewardsPageUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveRewardsPageUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_PAGE_UI_H_
