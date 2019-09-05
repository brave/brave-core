/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_p3a.h"

#include "base/metrics/histogram_macros.h"

namespace brave_rewards {

void RecordWalletBalanceP3A(bool wallet_created, size_t b) {
  int answer = 0;
  if (wallet_created) {
    if (b < 10) {
      answer = 1;
    } else if (10 <= b && b < 50) {
      answer = 2;
    } else if (50 <= b) {
      answer = 3;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.WalletBalance", answer, 3);
}

void RecordAutoContributionsState(AutoContributionsP3AState state, int count) {
  DCHECK_GE(count, 0);
  int answer = 0;
  switch (state) {
    case AutoContributionsP3AState::kNoWallet:
      break;
    case AutoContributionsP3AState::kWalletCreatedAutoContributeOff:
      answer = 1;
      break;
    case AutoContributionsP3AState::kAutoContributeOn:
      switch (count) {
        case 0:
          answer = 2;
          break;
        case 1:
          answer = 3;
          break;
        default:
          answer = 4;
      }
      break;
    default:
      NOTREACHED();
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.AutoContributionsState", answer, 4);
}

void RecordTipsState(bool wallet_created,
                     int one_time_count,
                     int recurring_count) {
  DCHECK_GE(one_time_count, 0);
  DCHECK_GE(recurring_count, 0);

  int answer = 0;
  if (wallet_created) {
    if (one_time_count == 0 && recurring_count == 0) {
      answer = 1;
    } else if (one_time_count > 0 && recurring_count > 0) {
      answer = 4;
    } else if (one_time_count > 0) {
      answer = 2;
    } else {
      answer = 3;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.TipsState", answer, 4);
}

void RecordAdsState(AdsP3AState state) {
  UMA_HISTOGRAM_ENUMERATION("Brave.Rewards.AdsState", state);
}

void RecordNoWalletCreatedForAllMetrics() {
  RecordWalletBalanceP3A(false, 0);
  RecordAutoContributionsState(AutoContributionsP3AState::kNoWallet, 0);
  RecordTipsState(false, 0, 0);
  RecordAdsState(AdsP3AState::kNoWallet);
}

}  // namespace brave_rewards
