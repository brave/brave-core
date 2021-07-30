/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_

#include <stddef.h>
#include <ctype.h>

#include <string>

#include "base/containers/flat_map.h"
#include "base/strings/string_piece.h"

class PrefService;

namespace base {
class DictionaryValue;
}

namespace brave_rewards {
namespace p3a {

struct WalletState {
  bool wallet_created = false;
  bool rewards_enabled = false;
  bool grants_claimed = false;
  bool funds_added = false;
};

void RecordWalletState(const WalletState& state);

void RecordWalletBalance(bool wallet_created,
                         bool rewards_enabled,
                         size_t balance);

enum class AutoContributionsState {
  kNoWallet,
  kRewardsDisabled,
  kWalletCreatedAutoContributeOff,
  kAutoContributeOn,
};

void RecordAutoContributionsState(AutoContributionsState state, int count);

void RecordTipsState(bool wallet_created,
                     bool rewards_enabled,
                     int one_time_count,
                     int recurring_count);

enum class AdsState {
  kNoWallet,
  kRewardsDisabled,
  kAdsDisabled,
  kAdsEnabled,
  kAdsEnabledThenDisabledRewardsOn,
  kAdsEnabledThenDisabledRewardsOff,
  kMaxValue = kAdsEnabledThenDisabledRewardsOff,
};

void RecordAdsState(AdsState state);

void UpdateAdsStateOnPreferenceChange(PrefService* prefs,
                                      const std::string& pref);

// Records an initial metric state ("disabled" or "enabled") if it was not done
// before. Intended to be called if the user has already created a wallet.
void MaybeRecordInitialAdsState(PrefService* local_state);

void RecordNoWalletCreatedForAllMetrics();

void RecordRewardsDisabledForSomeMetrics();

double CalcWalletBalance(base::flat_map<std::string, double> wallets,
                         double user_funds);

void ExtractAndLogStats(const base::DictionaryValue& dict);

}  // namespace p3a
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
