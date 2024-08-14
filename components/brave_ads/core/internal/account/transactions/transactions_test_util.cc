/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

namespace brave_ads::test {

TransactionInfo BuildTransaction(const double value,
                                 const AdType ad_type,
                                 const ConfirmationType confirmation_type,
                                 const base::Time reconciled_at,
                                 const bool should_generate_random_uuids) {
  TransactionInfo transaction;

  transaction.id = RandomUuidOr(should_generate_random_uuids, kTransactionId);
  transaction.created_at = Now();
  transaction.creative_instance_id =
      RandomUuidOr(should_generate_random_uuids, kCreativeInstanceId);
  transaction.segment = kSegment;
  transaction.value = value;
  transaction.ad_type = ad_type;
  transaction.confirmation_type = confirmation_type;
  if (!reconciled_at.is_null()) {
    transaction.reconciled_at = reconciled_at;
  }

  return transaction;
}

TransactionInfo BuildUnreconciledTransaction(
    const double value,
    const AdType ad_type,
    const ConfirmationType confirmation_type,
    const bool should_generate_random_uuids) {
  return BuildTransaction(value, ad_type, confirmation_type,
                          /*reconciled_at=*/{}, should_generate_random_uuids);
}

}  // namespace brave_ads::test
