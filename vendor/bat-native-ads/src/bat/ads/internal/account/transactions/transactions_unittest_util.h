/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_UNITTEST_UTIL_H_

#include "bat/ads/transaction_info_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class ConfirmationType;
struct TransactionInfo;

void SaveTransactions(const TransactionList& transactions);

TransactionInfo BuildTransaction(const double value,
                                 const ConfirmationType& confirmation_type,
                                 const base::Time reconciled_at);

TransactionInfo BuildTransaction(const double value,
                                 const ConfirmationType& confirmation_type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_UNITTEST_UTIL_H_
