/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_EARNINGS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_EARNINGS_UTIL_H_

#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"
#include "bat/ads/transaction_info_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {
namespace rewards {

double GetUnreconciledEarningsForDateRange(
    const TransactionList& transaction_history,
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
    const base::Time& from_time,
    const base::Time& to_time);

}  // namespace rewards
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_EARNINGS_UTIL_H_
