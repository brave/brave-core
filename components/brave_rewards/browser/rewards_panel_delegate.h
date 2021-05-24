/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PANEL_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PANEL_DELEGATE_H_

class Profile;

namespace brave_rewards {

class RewardsPanelDelegate {
 public:
  virtual ~RewardsPanelDelegate() = default;

  virtual bool ShowRewardsPanel(Profile* profile) = 0;
  virtual bool ShowAdaptiveCaptchaPanel(Profile* profile) = 0;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PANEL_DELEGATE_H_
