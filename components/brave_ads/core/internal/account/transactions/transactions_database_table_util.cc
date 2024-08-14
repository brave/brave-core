/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::database {

void PurgeExpiredTransactions() {
  const database::table::Transactions database_table;
  database_table.PurgeExpired(base::BindOnce([](const bool success) {
    if (!success) {
      return BLOG(0, "Failed to purge expired transactions");
    }

    BLOG(3, "Successfully purged expired transactions");
  }));
}

void SaveTransactions(const TransactionList& transactions) {
  database::table::Transactions database_table;
  database_table.Save(transactions, base::BindOnce([](const bool success) {
                        if (!success) {
                          return BLOG(0, "Failed to save transactions");
                        }

                        BLOG(3, "Successfully saved transactions");
                      }));
}

}  // namespace brave_ads::database
