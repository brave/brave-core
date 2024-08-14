/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

namespace brave_ads {

TransactionInfo BuildTransaction(const std::string& creative_instance_id,
                                 const std::string& segment,
                                 const double value,
                                 const AdType ad_type,
                                 const ConfirmationType confirmation_type) {
  CHECK(!creative_instance_id.empty());
  CHECK_NE(AdType::kUndefined, ad_type);
  CHECK_NE(ConfirmationType::kUndefined, confirmation_type);

  TransactionInfo transaction;
  transaction.id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  transaction.created_at = base::Time::Now();
  transaction.creative_instance_id = creative_instance_id;
  transaction.value = value;
  transaction.segment = segment;
  transaction.ad_type = ad_type;
  transaction.confirmation_type = confirmation_type;

  return transaction;
}

TransactionInfo AddTransaction(const std::string& creative_instance_id,
                               const std::string& segment,
                               const double value,
                               const AdType ad_type,
                               const ConfirmationType confirmation_type,
                               AddTransactionCallback callback) {
  CHECK(!creative_instance_id.empty());
  CHECK_NE(AdType::kUndefined, ad_type);
  CHECK_NE(ConfirmationType::kUndefined, confirmation_type);

  const TransactionInfo transaction = BuildTransaction(
      creative_instance_id, segment, value, ad_type, confirmation_type);

  database::table::Transactions database_table;
  database_table.Save(
      {transaction},
      base::BindOnce(
          [](AddTransactionCallback callback,
             const TransactionInfo& transaction, const bool success) {
            std::move(callback).Run(success, transaction);
          },
          std::move(callback), transaction));

  return transaction;
}

void GetTransactionsForDateRange(const base::Time from_time,
                                 const base::Time to_time,
                                 GetTransactionsCallback callback) {
  const database::table::Transactions database_table;
  database_table.GetForDateRange(
      from_time, to_time,
      base::BindOnce(
          [](GetTransactionsCallback callback, const bool success,
             const TransactionList& transactions) {
            std::move(callback).Run(success, transactions);
          },
          std::move(callback)));
}

}  // namespace brave_ads
