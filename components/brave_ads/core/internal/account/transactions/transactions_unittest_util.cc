/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads::test {

void SaveTransactions(const TransactionList& transactions) {
  database::table::Transactions database_table;
  database_table.Save(
      transactions, base::BindOnce([](const bool success) { CHECK(success); }));
}

TransactionInfo BuildTransaction(const double value,
                                 const AdType ad_type,
                                 const ConfirmationType confirmation_type,
                                 const base::Time reconciled_at,
                                 const bool should_use_random_uuids) {
  TransactionInfo transaction;

  transaction.id = GetConstantId(should_use_random_uuids, kTransactionId);
  transaction.created_at = Now();
  transaction.creative_instance_id =
      GetConstantId(should_use_random_uuids, kCreativeInstanceId);
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
    const bool should_use_random_uuids) {
  return BuildTransaction(value, ad_type, confirmation_type,
                          /*reconciled_at=*/{}, should_use_random_uuids);
}

}  // namespace brave_ads::test
