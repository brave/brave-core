/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"

#include <utility>

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

std::optional<ConfirmationInfo> BuildNonRewardConfirmation(
    const TransactionInfo& transaction,
    base::Value::Dict user_data) {
  CHECK(transaction.IsValid());
  CHECK(!UserHasJoinedBraveRewards());

  ConfirmationInfo confirmation;
  confirmation.transaction_id = transaction.id;
  confirmation.creative_instance_id = transaction.creative_instance_id;
  confirmation.type = transaction.confirmation_type;
  confirmation.ad_type = transaction.ad_type;
  confirmation.created_at = transaction.created_at;
  confirmation.user_data =
      BuildConfirmationUserData(transaction, std::move(user_data));

  return confirmation;
}

}  // namespace brave_ads
