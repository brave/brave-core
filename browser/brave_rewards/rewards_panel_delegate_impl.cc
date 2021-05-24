/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/brave_rewards/rewards_panel_delegate_impl.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "chrome/browser/ui/browser_finder.h"

namespace {
constexpr char kAdaptiveCaptchaPanelUrl[] = "adaptive_captcha_panel.html";
constexpr char kRewardsPanelUrl[] = "brave_rewards_panel.html";

bool ShowPanel(Profile* profile, const std::string& relative_path) {
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  if (!browser) {
    return false;
  }

  std::string error;
  bool popup_shown = extensions::BraveActionAPI::ShowActionUI(
      browser, brave_rewards_extension_id,
      std::make_unique<std::string>(relative_path), &error);
  if (!popup_shown) {
    return false;
  }

  return true;
}

}  // namespace

namespace brave_rewards {

bool RewardsPanelDelegateImpl::ShowRewardsPanel(Profile* profile) {
  return ShowPanel(profile, kRewardsPanelUrl);
}

bool RewardsPanelDelegateImpl::ShowAdaptiveCaptchaPanel(Profile* profile) {
  return ShowPanel(profile, kAdaptiveCaptchaPanelUrl);
}

}  // namespace brave_rewards
