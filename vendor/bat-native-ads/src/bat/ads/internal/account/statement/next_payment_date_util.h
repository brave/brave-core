/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_STATEMENT_NEXT_PAYMENT_DATE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_STATEMENT_NEXT_PAYMENT_DATE_UTIL_H_

#include "bat/ads/transaction_info_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

base::Time CalculateNextPaymentDate(const base::Time& next_token_redemption_at,
                                    const TransactionList& transactions);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_STATEMENT_NEXT_PAYMENT_DATE_UTIL_H_
