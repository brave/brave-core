/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_

#include <stddef.h>

namespace brave_rewards {

void RecordWalletBalanceP3A(bool wallet_created, size_t balance);

enum class AutoContributionsP3AState {
  kNoWallet,
  kWalletCreatedAutoContributeOff,
  kAutoContributeOn,
};

void RecordAutoContributionsState(AutoContributionsP3AState state,
                                  int count);

void RecordTipsState(bool wallet_created, int one_time_count,
                     int recurring_count);

enum class AdsP3AState {
  kNoWallet,
  kAdsDisabled,
  kAdsEnabled,
  kAdsEnabledThenDisabledRewardsOn,
  kAdsEnabledThenDisabledRewardsOff,
  kMaxValue = kAdsEnabledThenDisabledRewardsOff,
};

void RecordAdsState(AdsP3AState state);

void RecordNoWalletCreatedForAllMetrics();

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
