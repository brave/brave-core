/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CONFIRMATION_DIAGNOSTICS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CONFIRMATION_DIAGNOSTICS_UTIL_H_

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"

namespace brave_ads {

void GetConfirmationQueueCallback(
    GetConfirmationQueueDiagnosticsCallback callback,
    bool success,
    const ConfirmationQueueItemList& confirmation_queue_items);

void GetPaymentTokensCallback(GetPaymentTokensDiagnosticsCallback callback,
                              bool success,
                              const PaymentTokenList& payment_tokens);

void GetTransactionsCallback(GetTransactionsDiagnosticsCallback callback,
                             bool success,
                             const TransactionList& transactions);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CONFIRMATION_DIAGNOSTICS_UTIL_H_
