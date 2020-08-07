/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_p3a.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards {

void RecordWalletBalanceP3A(bool wallet_created, bool rewards_enabled,
                            size_t b) {
  int answer = 0;
  if (wallet_created && !rewards_enabled) {
    answer = 1;
  } else if (rewards_enabled) {
    DCHECK(wallet_created);
    if (b < 10) {
      answer = 2;
    } else if (10 <= b && b < 50) {
      answer = 3;
    } else if (50 <= b) {
      answer = 4;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.WalletBalance.2", answer, 4);
}

void RecordAutoContributionsState(AutoContributionsP3AState state, int count) {
  DCHECK_GE(count, 0);
  int answer = 0;
  switch (state) {
    case AutoContributionsP3AState::kNoWallet:
      break;
    case AutoContributionsP3AState::kRewardsDisabled:
      answer = 1;
      break;
    case AutoContributionsP3AState::kWalletCreatedAutoContributeOff:
      answer = 2;
      break;
    case AutoContributionsP3AState::kAutoContributeOn:
      switch (count) {
        case 0:
          answer = 3;
          break;
        case 1:
          answer = 4;
          break;
        default:
          answer = 5;
      }
      break;
    default:
      NOTREACHED();
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.AutoContributionsState.2", answer,
                             5);
}

void RecordTipsState(bool wallet_created,
                     bool rewards_enabled,
                     int one_time_count,
                     int recurring_count) {
  DCHECK_GE(one_time_count, 0);
  DCHECK_GE(recurring_count, 0);

  int answer = 0;
  if (wallet_created && !rewards_enabled) {
    answer = 1;
  } else if (rewards_enabled) {
    DCHECK(wallet_created);
    if (one_time_count == 0 && recurring_count == 0) {
      answer = 2;
    } else if (one_time_count > 0 && recurring_count > 0) {
      answer = 5;
    } else if (one_time_count > 0) {
      answer = 3;
    } else {
      answer = 4;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.TipsState.2", answer, 5);
}

void RecordAdsState(AdsP3AState state) {
  UMA_HISTOGRAM_ENUMERATION("Brave.Rewards.AdsState.2", state);
}

void UpdateAdsP3AOnPreferenceChange(PrefService *prefs,
                                    const std::string& pref) {
  using brave_rewards::AdsP3AState;
  const bool rewards_enabled =
      prefs->GetBoolean(brave_rewards::prefs::kBraveRewardsEnabled);
  const bool ads_enabled = prefs->GetBoolean(brave_ads::prefs::kEnabled);
  if (pref == brave_ads::prefs::kEnabled) {
    if (ads_enabled) {
      brave_rewards::RecordAdsState(AdsP3AState::kAdsEnabled);
      prefs->SetBoolean(brave_ads::prefs::kAdsWereDisabled, false);
    } else {
      // Apparently, the pref was disabled.
      // TODO(ifremov): DCHECK(rewards_enabled)?
      brave_rewards::RecordAdsState(
          rewards_enabled ? AdsP3AState::kAdsEnabledThenDisabledRewardsOn :
                            AdsP3AState::kAdsEnabledThenDisabledRewardsOff);
      prefs->SetBoolean(brave_ads::prefs::kAdsWereDisabled, true);
    }
  } else if (pref == brave_rewards::prefs::kBraveRewardsEnabled) {
    // Rewards pref was changed.
    if (prefs->GetBoolean(brave_ads::prefs::kAdsWereDisabled)) {
      DCHECK(!ads_enabled);
      brave_rewards::RecordAdsState(
          rewards_enabled ? AdsP3AState::kAdsEnabledThenDisabledRewardsOn :
                            AdsP3AState::kAdsEnabledThenDisabledRewardsOff);
    } else if (!rewards_enabled) {
      // Ads state had never been disabled, but the user has toggled rewards
      // off.
      brave_rewards::RecordAdsState(AdsP3AState::kRewardsDisabled);
    } else {
      RecordAdsState(ads_enabled ? AdsP3AState::kAdsEnabled :
                                   AdsP3AState::kAdsDisabled);
    }
  }
}

void MaybeRecordInitialAdsP3AState(PrefService* prefs) {
  if (!prefs->GetBoolean(brave_ads::prefs::kHasAdsP3AState)) {
    const bool ads_state = prefs->GetBoolean(brave_ads::prefs::kEnabled);
    RecordAdsState(ads_state ? AdsP3AState::kAdsEnabled
                             : AdsP3AState::kAdsDisabled);
    prefs->SetBoolean(brave_ads::prefs::kHasAdsP3AState, true);
  }
}

void RecordNoWalletCreatedForAllMetrics() {
  RecordWalletBalanceP3A(false, false, 0);
  RecordAutoContributionsState(AutoContributionsP3AState::kNoWallet, 0);
  RecordTipsState(false, false, 0, 0);
  RecordAdsState(AdsP3AState::kNoWallet);
}

void RecordRewardsDisabledForSomeMetrics() {
  RecordWalletBalanceP3A(true, false, 1);
  RecordAutoContributionsState(AutoContributionsP3AState::kRewardsDisabled, 0);
  RecordTipsState(true, false, 0, 0);
  // Ads state is handled separately.
}


double CalcWalletBalanceForP3A(base::flat_map<std::string, double> wallets,
                               std::string user_funds) {
  double balance_minus_grant = 0.0;
  for (const auto& wallet : wallets) {
    // Skip anonymous wallet, since it can contain grants.
    if (wallet.first == "anonymous") {
      continue;
    }
    balance_minus_grant += static_cast<size_t>(wallet.second);
  }

  // `user_funds` is the amount of user-funded BAT
  // in the anonymous wallet (ex: not grants).
  double user_funds_value;
  balance_minus_grant +=
      base::StringToDouble(user_funds, &user_funds_value);
  return balance_minus_grant;
}

uint64_t RoundProbiToUint64(base::StringPiece probi) {
  if (probi.size() < 18) return 0;
  uint64_t grant = 0;
  base::StringToUint64(probi.substr(0, probi.size() - 18), &grant);
  return grant;
}

void ExtractAndLogP3AStats(const base::DictionaryValue& dict) {
  const base::Value* probi_value =
      dict.FindPath({"walletProperties", "probi_"});
  if (!probi_value || !probi_value->is_string()) {
    LOG(WARNING) << "Bad ledger state";
    return;
  }

  // Get grants.
  const base::Value* grants_value = dict.FindKey("grants");
  uint64_t total_grants = 0;
  if (grants_value) {
    if (!grants_value->is_list()) {
      LOG(WARNING) << "Bad grant value in ledger_state.";
    } else {
      const auto& grants = grants_value->GetList();
      // Sum all grants.
      for (const auto& grant : grants) {
        if (!grant.is_dict()) {
          LOG(WARNING) << "Bad grant value in ledger_state.";
          continue;
        }
        const base::Value* grant_amount = grant.FindKey("probi_");
        const base::Value* grant_currency = grant.FindKey("altcurrency");
        if (grant_amount->is_string() && grant_currency->is_string() &&
            grant_currency->GetString() == "BAT") {
          // Some kludge computations because we don't want to be very precise
          // for P3A purposes. Assuming grants can't be negative and are
          // greater than 1 BAT.
          const std::string& grant_str = grant_amount->GetString();
          total_grants += RoundProbiToUint64(grant_str);
        }
      }
    }
  }
  const uint64_t total =
      RoundProbiToUint64(probi_value->GetString()) - total_grants;
  RecordWalletBalanceP3A(true, true, total);
}

}  // namespace brave_rewards
