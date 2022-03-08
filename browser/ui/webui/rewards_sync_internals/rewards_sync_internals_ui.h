/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_REWARDS_SYNC_INTERNALS_REWARDS_SYNC_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_REWARDS_SYNC_INTERNALS_REWARDS_SYNC_INTERNALS_UI_H_

#include "content/public/browser/web_ui_controller.h"

// The implementation for the brave://rewards-sync-internals page.
class RewardsSyncInternalsUI : public content::WebUIController {
 public:
  explicit RewardsSyncInternalsUI(content::WebUI* web_ui);

  RewardsSyncInternalsUI(const RewardsSyncInternalsUI&) = delete;
  RewardsSyncInternalsUI& operator=(const RewardsSyncInternalsUI&) = delete;

  ~RewardsSyncInternalsUI() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_REWARDS_SYNC_INTERNALS_REWARDS_SYNC_INTERNALS_UI_H_
