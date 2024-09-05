/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_TRANSACTION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_TRANSACTION_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database {

// Returns true if the transaction result is a success.
bool IsSuccess(
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result);

// Returns true if the transaction result is an error.
bool IsError(
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result);

// Run a database transaction.
void RunDBTransaction(mojom::DBTransactionInfoPtr mojom_db_transaction,
                      ResultCallback callback);

// Raze the database. This must be done before any other actions are run. All
// tables must be recreated after the raze operation is completed.
void Raze(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

// Execute a SQL statement.
void Execute(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
             const std::string& sql);

// Execute a SQL statement with placeholders.
void Execute(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
             const std::string& sql,
             const std::vector<std::string>& subst);

// Vacuum the database. This must be done after any other actions are run.
void Vacuum(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_TRANSACTION_UTIL_H_
