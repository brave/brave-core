/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

using AddTransactionCallback =
    base::OnceCallback<void(bool success, const TransactionInfo& transaction)>;

using GetTransactionsCallback =
    base::OnceCallback<void(bool success, const TransactionList& transactions)>;

using RemoveAllTransactionsCallback = base::OnceCallback<void(bool success)>;

class AdType;
class ConfirmationType;

TransactionInfo AddTransaction(const std::string& creative_instance_id,
                               const std::string& segment,
                               double value,
                               const AdType& ad_type,
                               const ConfirmationType& confirmation_type,
                               AddTransactionCallback callback);

void GetTransactionsForDateRange(base::Time from_time,
                                 base::Time to_time,
                                 GetTransactionsCallback callback);

void RemoveAllTransactions(RemoveAllTransactionsCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
