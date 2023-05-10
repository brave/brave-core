/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_user_data_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

namespace brave_ads {

absl::optional<ConfirmationInfo> BuildConfirmation(
    privacy::TokenGeneratorInterface* token_generator,
    const TransactionInfo& transaction) {
  CHECK(token_generator);
  return CreateConfirmation(token_generator, transaction, /*user_data*/ {});
}

absl::optional<ConfirmationInfo> BuildConfirmation(
    privacy::TokenGeneratorInterface* token_generator) {
  CHECK(token_generator);

  TransactionInfo transaction;
  transaction.id = kTransactionId;
  transaction.creative_instance_id = kCreativeInstanceId;
  transaction.confirmation_type = ConfirmationType::kViewed;
  transaction.ad_type = AdType::kNotificationAd;
  transaction.created_at = Now();

  return BuildConfirmation(token_generator, transaction);
}

}  // namespace brave_ads
