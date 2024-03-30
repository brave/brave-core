/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"

#include <utility>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"

namespace brave_rewards {

RewardsPanelCoordinator::RewardsPanelCoordinator(Browser* browser)
    : BrowserUserData<RewardsPanelCoordinator>(*browser) {}

RewardsPanelCoordinator::~RewardsPanelCoordinator() = default;

bool RewardsPanelCoordinator::IsRewardsPanelURLForTesting(const GURL& url) {
  return url.host() == kBraveRewardsPanelHost;
}

bool RewardsPanelCoordinator::OpenRewardsPanel() {
  return OpenWithArgs(
      mojom::RewardsPanelArgs(mojom::RewardsPanelView::kDefault, ""));
}

bool RewardsPanelCoordinator::ShowRewardsSetup() {
  return OpenWithArgs(
      mojom::RewardsPanelArgs(mojom::RewardsPanelView::kRewardsSetup, ""));
}

bool RewardsPanelCoordinator::ShowAdaptiveCaptcha() {
  return OpenWithArgs(
      mojom::RewardsPanelArgs(mojom::RewardsPanelView::kAdaptiveCaptcha, ""));
}

void RewardsPanelCoordinator::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void RewardsPanelCoordinator::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool RewardsPanelCoordinator::OpenWithArgs(mojom::RewardsPanelArgs&& args) {
  if (GetBrowser().window()->IsMinimized()) {
    GetBrowser().window()->Restore();
  }

  panel_args_ = std::move(args);

  for (auto& observer : observers_) {
    observer.OnRewardsPanelRequested(panel_args_);
  }

  return !observers_.empty();
}

BROWSER_USER_DATA_KEY_IMPL(RewardsPanelCoordinator);

}  // namespace brave_rewards
