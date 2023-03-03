/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class AdType;
class ConfirmationType;

namespace transactions {

using AddCallback =
    base::OnceCallback<void(const bool, const TransactionInfo& transaction)>;

using GetCallback =
    base::OnceCallback<void(const bool, const TransactionList&)>;

using RemoveAllCallback = base::OnceCallback<void(const bool)>;

TransactionInfo Add(const std::string& creative_instance_id,
                    double value,
                    const AdType& ad_type,
                    const ConfirmationType& confirmation_type,
                    AddCallback callback);

void GetForDateRange(base::Time from_time,
                     base::Time to_time,
                     GetCallback callback);

void RemoveAll(RemoveAllCallback callback);

}  // namespace transactions
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_H_
