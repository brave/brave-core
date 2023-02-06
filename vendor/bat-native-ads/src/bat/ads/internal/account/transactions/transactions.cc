/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/transactions/transactions_database_table.h"

namespace ads::transactions {

TransactionInfo Add(const std::string& creative_instance_id,
                    const double value,
                    const AdType& ad_type,
                    const ConfirmationType& confirmation_type,
                    AddCallback callback) {
  DCHECK(!creative_instance_id.empty());
  DCHECK_NE(AdType::kUndefined, ad_type.value());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type.value());

  TransactionInfo transaction;
  transaction.id = base::GUID::GenerateRandomV4().AsLowercaseString();
  transaction.created_at = base::Time::Now();
  transaction.creative_instance_id = creative_instance_id;
  transaction.ad_type = ad_type;
  transaction.confirmation_type = confirmation_type;
  transaction.value = value;

  database::table::Transactions database_table;
  database_table.Save(
      {transaction},
      base::BindOnce(
          [](AddCallback callback, const TransactionInfo& transaction,
             const bool success) {
            if (!success) {
              std::move(callback).Run(/*success*/ false, {});
              return;
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
              std::move(callback).Run(/*success*/ false, {});
              return;
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
          std::move(callback).Run(/*success*/ false);
          return;
        }

        std::move(callback).Run(/*success*/ true);
      },
      std::move(callback)));
}

}  // namespace ads::transactions
