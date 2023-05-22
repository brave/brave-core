/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_news/common/pref_names.h"

namespace brave_ads {

namespace {

bool UserHasOptedInToBraveNews() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
             brave_news::prefs::kBraveNewsOptedIn) &&
         AdsClientHelper::GetInstance()->GetBooleanPref(
             brave_news::prefs::kNewTabPageShowToday);
}

}  // namespace

bool UserHasOptedIn() {
  return UserHasOptedInToBravePrivateAds() || UserHasOptedInToBraveNews();
}

bool UserHasOptedInToBravePrivateAds() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(prefs::kEnabled);
}

bool ShouldRewardUser() {
  return UserHasOptedInToBravePrivateAds();
}

void ResetRewards(ResetRewardsCallback callback) {
  RemoveAllTransactions(base::BindOnce(
      [](ResetRewardsCallback callback, const bool success) {
        if (!success) {
          BLOG(0, "Failed to remove transactions");
          return std::move(callback).Run(/*success*/ false);
        }

        ResetConfirmations();

        std::move(callback).Run(/*success*/ true);
      },
      std::move(callback)));
}

}  // namespace brave_ads
