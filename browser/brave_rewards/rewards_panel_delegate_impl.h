/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_DELEGATE_IMPL_H_

#include "brave/components/brave_rewards/browser/rewards_panel_delegate.h"

namespace brave_rewards {

class RewardsPanelDelegateImpl : public RewardsPanelDelegate {
 public:
  RewardsPanelDelegateImpl() = default;
  ~RewardsPanelDelegateImpl() override = default;

  RewardsPanelDelegateImpl(const RewardsPanelDelegateImpl&) = delete;
  RewardsPanelDelegateImpl& operator=(const RewardsPanelDelegateImpl&) = delete;

  // RewardsPanelDelegate:
  bool ShowRewardsPanel(Profile* profile) override;
  bool ShowAdaptiveCaptchaPanel(Profile* profile) override;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_DELEGATE_IMPL_H_
