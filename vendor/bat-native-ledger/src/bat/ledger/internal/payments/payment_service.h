/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_PAYMENT_SERVICE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_PAYMENT_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/internal/external_wallet/external_wallet_manager.h"
#include "bat/ledger/internal/payments/payment_data.h"

namespace ledger {

class PaymentService : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "payment-service";

  Future<absl::optional<PaymentOrder>> CreateOrder(
      const std::map<std::string, int>& items);

  Future<absl::optional<PaymentOrder>> GetOrder(const std::string& order_id);

  Future<bool> PostExternalTransaction(const std::string& order_id,
                                       const std::string& transaction_id,
                                       ExternalWalletProvider provider);

  Future<bool> PostCredentials(const std::string& order_id,
                               const std::string& item_id,
                               PaymentCredentialType type,
                               const std::vector<std::string>& blinded_tokens);

  Future<absl::optional<PaymentCredentials>> GetCredentials(
      const std::string& order_id,
      const std::string& item_id);

  Future<bool> PostPublisherVotes(const std::string& publisher_id,
                                  PaymentVoteType vote_type,
                                  const std::vector<PaymentVote>& votes);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_PAYMENT_SERVICE_H_
