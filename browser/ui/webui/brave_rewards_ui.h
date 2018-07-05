/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_UI_H_

#include "brave/browser/ui/webui/basic_ui.h"

class BraveRewardsUI : public BasicUI {
 public:
  BraveRewardsUI(content::WebUI* web_ui, const std::string& host);
  ~BraveRewardsUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveRewardsUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_UI_H_
