/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account_util.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/pref_names.h"

namespace ads {

bool ShouldRewardUser() {
  return AdsClientHelper::Get()->GetBooleanPref(prefs::kEnabled);
}

void ResetRewards(ResetRewardsCallback callback) {
  transactions::RemoveAll([=](const bool success) {
    if (!success) {
      BLOG(0, "Failed to remove transactions");
      callback(/* success */ false);
      return;
    }

    ConfirmationsState::Get()->reset_failed_confirmations();

    privacy::UnblindedPaymentTokens* unblinded_payment_tokens =
        ConfirmationsState::Get()->get_unblinded_payment_tokens();
    unblinded_payment_tokens->RemoveAllTokens();

    ConfirmationsState::Get()->Save();

    callback(/* success */ true);
  });
}

}  // namespace ads
