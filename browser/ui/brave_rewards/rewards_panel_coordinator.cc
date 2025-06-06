/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"

#include <utility>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

namespace brave_rewards {

RewardsPanelCoordinator::RewardsPanelCoordinator(
    BrowserWindowInterface* browser_window_interface)
    : browser_window_interface_(browser_window_interface) {
  DCHECK(browser_window_interface_);
}

RewardsPanelCoordinator::~RewardsPanelCoordinator() = default;

bool RewardsPanelCoordinator::IsRewardsPanelURLForTesting(const GURL& url) {
  return url.host() == kRewardsPageTopHost;
}

bool RewardsPanelCoordinator::OpenRewardsPanel() {
  if (browser_window_interface_->IsMinimized()) {
    // TODO(zenparsing): Available in CR138
    // browser_window_interface_->ActivateWindow();
  }

  for (auto& observer : observers_) {
    observer.OnRewardsPanelRequested();
  }

  return !observers_.empty();
}

void RewardsPanelCoordinator::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void RewardsPanelCoordinator::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_rewards
