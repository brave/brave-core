/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_UNITTEST_UTIL_H_

#include "bat/ads/internal/account/transactions/transaction_info.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class ConfirmationType;

void SaveTransactions(const TransactionList& transactions);

TransactionInfo BuildTransaction(double value,
                                 const ConfirmationType& confirmation_type,
                                 base::Time reconciled_at);

TransactionInfo BuildTransaction(double value,
                                 const ConfirmationType& confirmation_type);

int GetTransactionCount();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_UNITTEST_UTIL_H_
