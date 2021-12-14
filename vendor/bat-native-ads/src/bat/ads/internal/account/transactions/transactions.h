/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_

#include <string>

#include "bat/ads/internal/account/transactions/transactions_aliases.h"
#include "bat/ads/internal/database/tables/transactions_database_table_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class AdType;
class ConfirmationType;
struct TransactionInfo;

namespace transactions {

TransactionInfo Add(const std::string& creative_instance_id,
                    const double value,
                    const AdType& ad_type,
                    const ConfirmationType& confirmation_type,
                    AddTransactionCallback callback);

void GetForDateRange(const base::Time& from_time,
                     const base::Time& to_time,
                     GetTransactionsCallback callback);

void RemoveAll(RemoveAllTransactionsCallback callback);

}  // namespace transactions
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
