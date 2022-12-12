/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account_util.h"

#include "bat/ads/internal/account/confirmations/confirmation_util.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads {

bool ShouldRewardUser() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(prefs::kEnabled);
}

void ResetRewards(const ResetRewardsCallback& callback) {
  transactions::RemoveAll([callback](const bool success) {
    if (!success) {
      BLOG(0, "Failed to remove transactions");
      callback(/*success*/ false);
      return;
    }

    ResetConfirmations();

    callback(/*success*/ true);
  });
}

}  // namespace ads
