/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_test_util.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

std::optional<ConfirmationInfo> BuildNonRewardConfirmation(
    const bool should_generate_random_uuids) {
  CHECK(!UserHasJoinedBraveRewards());

  const TransactionInfo transaction = BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, should_generate_random_uuids);
  return BuildNonRewardConfirmation(transaction, /*user_data=*/{});
}

}  // namespace brave_ads::test
