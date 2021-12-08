/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/database/tables/transactions_database_table.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/transaction_info.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace ads {

void SaveTransactions(const TransactionList& transactions) {
  database::table::Transactions database_table;
  database_table.Save(transactions,
                      [](const bool success) { ASSERT_TRUE(success); });
}

TransactionInfo BuildTransaction(const double value,
                                 const ConfirmationType& confirmation_type,
                                 const base::Time& reconciled_at) {
  TransactionInfo transaction;

  transaction.id = base::GenerateGUID();
  transaction.created_at = Now().ToDoubleT();
  transaction.creative_instance_id = base::GenerateGUID();
  transaction.value = value;
  transaction.ad_type = AdType::kAdNotification;
  transaction.confirmation_type = confirmation_type;
  transaction.reconciled_at = reconciled_at.ToDoubleT();

  return transaction;
}

TransactionInfo BuildTransaction(const double value,
                                 const ConfirmationType& confirmation_type) {
  return BuildTransaction(value, confirmation_type, base::Time());
}

}  // namespace ads
