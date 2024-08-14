/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_UTIL_H_

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"

namespace brave_ads::database {

void PurgeExpiredTransactions();

void SaveTransactions(const TransactionList& transactions);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_UTIL_H_
