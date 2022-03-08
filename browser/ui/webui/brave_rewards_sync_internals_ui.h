/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_SYNC_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_SYNC_INTERNALS_UI_H_

#include "content/public/browser/web_ui_controller.h"

// The implementation for the brave://rewards-sync-internals page.
class BraveRewardsSyncInternalsUI : public content::WebUIController {
 public:
  explicit BraveRewardsSyncInternalsUI(content::WebUI* web_ui);

  BraveRewardsSyncInternalsUI(const BraveRewardsSyncInternalsUI&) = delete;
  BraveRewardsSyncInternalsUI& operator=(const BraveRewardsSyncInternalsUI&) =
      delete;

  ~BraveRewardsSyncInternalsUI() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_SYNC_INTERNALS_UI_H_
