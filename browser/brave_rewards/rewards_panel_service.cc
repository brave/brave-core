/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_panel_service.h"

#include <string>
#include <utility>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"

namespace brave_rewards {

RewardsPanelService::RewardsPanelService(Profile* profile)
    : profile_(profile) {}

RewardsPanelService::~RewardsPanelService() = default;

bool RewardsPanelService::OpenRewardsPanel() {
  return OpenRewardsPanel("");
}

bool RewardsPanelService::OpenRewardsPanel(const std::string& args) {
  auto* browser = chrome::FindTabbedBrowser(profile_, false);
  if (!browser) {
    return false;
  }

  if (browser->window()->IsMinimized()) {
    browser->window()->Restore();
  }

  panel_args_ = args;

  for (auto& observer : observers_) {
    observer.OnRewardsPanelRequested(browser);
  }

  return !observers_.empty();
}

bool RewardsPanelService::ShowAdaptiveCaptcha() {
  return OpenRewardsPanel("adaptive-captcha");
}

bool RewardsPanelService::ShowBraveTalkOptIn() {
  return OpenRewardsPanel("brave-talk-opt-in");
}

void RewardsPanelService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void RewardsPanelService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void RewardsPanelService::NotifyPanelClosed(Browser* browser) {
  DCHECK(browser);
  for (auto& observer : observers_) {
    observer.OnRewardsPanelClosed(browser);
  }
}

std::string RewardsPanelService::TakePanelArguments() {
  return std::move(panel_args_);
}

}  // namespace brave_rewards
