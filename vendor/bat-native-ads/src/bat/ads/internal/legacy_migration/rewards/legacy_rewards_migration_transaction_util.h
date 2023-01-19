/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_TRANSACTION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_TRANSACTION_UTIL_H_

#include "absl/types/optional.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/legacy_migration/rewards/payment_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads::rewards {

TransactionList GetAllUnreconciledTransactions(
    const TransactionList& transactions,
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens);

absl::optional<TransactionList>
BuildTransactionsForReconciledTransactionsThisMonth(
    const PaymentList& payments);

absl::optional<TransactionInfo>
BuildTransactionForReconciledTransactionsLastMonth(const PaymentList& payments);

}  // namespace ads::rewards

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_TRANSACTION_UTIL_H_
