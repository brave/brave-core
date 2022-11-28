/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/guid.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

namespace brave_ads::transactions {

TransactionInfo Get(const std::string& creative_instance_id,
                    const AdType& ad_type,
                    const ConfirmationType& confirmation_type,
                    const double value) {
  DCHECK(!creative_instance_id.empty());
  DCHECK_NE(AdType::kUndefined, ad_type);
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type);

  TransactionInfo transaction;
  transaction.id = base::GUID::GenerateRandomV4().AsLowercaseString();
  transaction.created_at = base::Time::Now();
  transaction.creative_instance_id = creative_instance_id;
  transaction.ad_type = ad_type;
  transaction.confirmation_type = confirmation_type;
  transaction.value = value;

  return transaction;
}

TransactionInfo Add(const std::string& creative_instance_id,
                    const AdType& ad_type,
                    const ConfirmationType& confirmation_type,
                    const double value,
                    AddCallback callback) {
  TransactionInfo transaction =
      Get(creative_instance_id, ad_type, confirmation_type, value);

  database::table::Transactions database_table;
  database_table.Save(
      {transaction},
      base::BindOnce(
          [](AddCallback callback, const TransactionInfo& transaction,
             const bool success) {
            if (!success) {
              return std::move(callback).Run(/*success*/ false, {});
            }

            std::move(callback).Run(/*success*/ true, transaction);
          },
          std::move(callback), transaction));

  return transaction;
}

void GetForDateRange(const base::Time from_time,
                     const base::Time to_time,
                     GetCallback callback) {
  const database::table::Transactions database_table;
  database_table.GetForDateRange(
      from_time, to_time,
      base::BindOnce(
          [](GetCallback callback, const bool success,
             const TransactionList& transactions) {
            if (!success) {
              return std::move(callback).Run(/*success*/ false, {});
            }

            std::move(callback).Run(/*success*/ true, transactions);
          },
          std::move(callback)));
}

void RemoveAll(RemoveAllCallback callback) {
  const database::table::Transactions database_table;
  database_table.Delete(base::BindOnce(
      [](RemoveAllCallback callback, const bool success) {
        if (!success) {
          return std::move(callback).Run(/*success*/ false);
        }

        std::move(callback).Run(/*success*/ true);
      },
      std::move(callback)));
}

}  // namespace brave_ads::transactions
