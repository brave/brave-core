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

void RecordWalletBalanceP3A(bool wallet_created, size_t balance);

enum class AutoContributionsP3AState {
  kNoWallet,
  kWalletCreatedAutoContributeOff,
  kAutoContributeOn,
};

void RecordAutoContributionsState(AutoContributionsP3AState state, int count);

void RecordTipsState(bool wallet_created,
                     int one_time_count,
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

void UpdateAdsP3AOnPreferenceChange(PrefService* prefs,
                                    const std::string& pref);

// Records an initial metric state ("disabled" or "enabled") if it was not done
// before. Intended to be called if the user has already created a wallet.
void MaybeRecordInitialAdsP3AState(PrefService* local_state);

void RecordNoWalletCreatedForAllMetrics();

double CalcWalletBalanceForP3A(base::flat_map<std::string, double> wallets,
                               std::string user_funds);

uint64_t RoundProbiToUint64(base::StringPiece probi);

void ExtractAndLogP3AStats(const base::DictionaryValue& dict);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
