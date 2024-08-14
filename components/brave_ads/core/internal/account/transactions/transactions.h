/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

using AddTransactionCallback =
    base::OnceCallback<void(bool success, const TransactionInfo& transaction)>;

using GetTransactionsCallback =
    base::OnceCallback<void(bool success, const TransactionList& transactions)>;

TransactionInfo BuildTransaction(const std::string& creative_instance_id,
                                 const std::string& segment,
                                 double value,
                                 AdType ad_type,
                                 ConfirmationType confirmation_type);

TransactionInfo AddTransaction(const std::string& creative_instance_id,
                               const std::string& segment,
                               double value,
                               AdType ad_type,
                               ConfirmationType confirmation_type,
                               AddTransactionCallback callback);

void GetTransactionsForDateRange(base::Time from_time,
                                 base::Time to_time,
                                 GetTransactionsCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
