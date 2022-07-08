/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_REWARDS_PANEL_EXTENSION_HANDLER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_REWARDS_PANEL_EXTENSION_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_rewards/rewards_panel/rewards_panel_coordinator.h"
#include "url/gurl.h"

namespace brave_rewards {

class RewardsService;

// Loads the Rewards extension if required and dispatches panel requests to the
// extension.
class RewardsPanelExtensionHandler : public RewardsPanelCoordinator::Observer {
 public:
  RewardsPanelExtensionHandler(Browser* browser,
                               RewardsService* rewards_service);

  ~RewardsPanelExtensionHandler() override;

  RewardsPanelExtensionHandler(const RewardsPanelExtensionHandler&) = delete;
  RewardsPanelExtensionHandler& operator=(const RewardsPanelExtensionHandler&) =
      delete;

  static bool IsRewardsExtensionPanelURL(const GURL& url);

  void OnRewardsPanelRequested(const mojom::RewardsPanelArgs& args) override;

 private:
  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<RewardsService> rewards_service_ = nullptr;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_REWARDS_PANEL_EXTENSION_HANDLER_H_
