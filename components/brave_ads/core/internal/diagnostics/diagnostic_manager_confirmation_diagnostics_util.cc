/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager_confirmation_diagnostics_util.h"

#include <optional>
#include <string>
#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"

namespace brave_ads {

namespace {

base::ListValue BuildConfirmationQueue(
    const ConfirmationQueueItemList& confirmation_queue_items) {
  base::ListValue list;
  list.reserve(confirmation_queue_items.size());

  for (const auto& confirmation_queue_item : confirmation_queue_items) {
    if (!confirmation_queue_item.IsValid()) {
      // Skip invalid confirmation queue items.
      continue;
    }

    const ConfirmationInfo& confirmation = confirmation_queue_item.confirmation;

    base::DictValue dict =
        base::DictValue()
            .Set("Transaction ID", confirmation.transaction_id)
            .Set("Ad Type", ToString(confirmation.ad_type))
            .Set("Confirmation Type", ToString(confirmation.type))
            .Set("Retry Count", confirmation_queue_item.retry_count);

    if (confirmation.created_at) {
      dict.Set("Created At",
               confirmation.created_at->InSecondsFSinceUnixEpoch());
    }

    if (confirmation_queue_item.process_at) {
      dict.Set("Process At",
               confirmation_queue_item.process_at->InSecondsFSinceUnixEpoch());
    }

    // The data sent to the server alongside this confirmation when redeemed;
    // surfaced here so it can be reviewed before that happens.
    base::DictValue user_data = confirmation.user_data.fixed.Clone();
    user_data.Merge(confirmation.user_data.dynamic.Clone());
    if (!user_data.empty()) {
      dict.Set("User Data", std::move(user_data));
    }

    list.Append(std::move(dict));
  }

  return list;
}

base::ListValue BuildPaymentTokens(const PaymentTokenList& payment_tokens) {
  base::ListValue list;
  list.reserve(payment_tokens.size());

  // Each payment token's BAT value is determined by which issuer public key
  // signed it, looked up the same way as `PaymentTokenIssuerPublicKeyExists`
  // (token_issuer_util.cc).
  const std::optional<IssuersInfo> issuers = GetIssuers();

  for (const auto& payment_token : payment_tokens) {
    if (!payment_token.IsValid()) {
      // Skip invalid payment tokens.
      continue;
    }

    base::DictValue dict =
        base::DictValue()
            .Set("Transaction ID", payment_token.transaction_id)
            .Set("Ad Type", ToString(payment_token.ad_type))
            .Set("Confirmation Type",
                 ToString(payment_token.confirmation_type));

    if (issuers) {
      if (const std::optional<std::string> public_key_base64 =
              payment_token.public_key.EncodeBase64()) {
        const auto iter =
            issuers->payment_token_issuer.public_keys.find(*public_key_base64);
        if (iter != issuers->payment_token_issuer.public_keys.cend()) {
          dict.Set("Value", iter->second);
        }
      }
    }

    list.Append(std::move(dict));
  }

  return list;
}

base::ListValue BuildTransactions(const TransactionList& transactions) {
  base::ListValue list;
  list.reserve(transactions.size());

  for (const auto& transaction : transactions) {
    if (!transaction.IsValid()) {
      // Skip invalid transactions.
      continue;
    }

    base::DictValue dict =
        base::DictValue()
            .Set("Transaction ID", transaction.id)
            .Set("Creative Instance ID", transaction.creative_instance_id)
            .Set("Ad Type", ToString(transaction.ad_type))
            .Set("Confirmation Type", ToString(transaction.confirmation_type))
            .Set("Value", transaction.value);

    if (transaction.created_at) {
      dict.Set("Created At",
               transaction.created_at->InSecondsFSinceUnixEpoch());
    }
    if (transaction.reconciled_at) {
      dict.Set("Reconciled At",
               transaction.reconciled_at->InSecondsFSinceUnixEpoch());
    }

    list.Append(std::move(dict));
  }

  return list;
}

}  // namespace

void GetConfirmationQueueCallback(
    GetConfirmationQueueDiagnosticsCallback callback,
    bool success,
    const ConfirmationQueueItemList& confirmation_queue_items) {
  std::move(callback).Run(success,
                          BuildConfirmationQueue(confirmation_queue_items));
}

void GetPaymentTokensCallback(GetPaymentTokensDiagnosticsCallback callback,
                              bool success,
                              const PaymentTokenList& payment_tokens) {
  std::move(callback).Run(success, BuildPaymentTokens(payment_tokens));
}

void GetTransactionsCallback(GetTransactionsDiagnosticsCallback callback,
                             bool success,
                             const TransactionList& transactions) {
  std::move(callback).Run(success, BuildTransactions(transactions));
}

}  // namespace brave_ads
