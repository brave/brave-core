/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "bat/ads/internal/account/confirmations/confirmation_util.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads {

bool ShouldRewardUser() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(prefs::kEnabled);
}

void ResetRewards(ResetRewardsCallback callback) {
  transactions::RemoveAll(base::BindOnce(
      [](ResetRewardsCallback callback, const bool success) {
        if (!success) {
          BLOG(0, "Failed to remove transactions");
          std::move(callback).Run(/*success*/ false);
          return;
        }

        ResetConfirmations();

        std::move(callback).Run(/*success*/ true);
      },
      std::move(callback)));
}

}  // namespace ads
