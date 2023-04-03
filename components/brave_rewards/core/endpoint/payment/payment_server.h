/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_SERVER_H_

#include <memory>

#include "brave/components/brave_rewards/core/endpoint/payment/get_credentials/get_credentials.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_credentials/post_credentials.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_order/post_order.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_uphold/post_transaction_uphold.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_votes/post_votes.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {

class PaymentServer {
 public:
  explicit PaymentServer(LedgerImpl* ledger);
  ~PaymentServer();

  payment::PostOrder* post_order() const;

  payment::PostCredentials* post_credentials() const;

  payment::GetCredentials* get_credentials() const;

  payment::PostVotes* post_votes() const;

  payment::PostTransactionGemini* post_transaction_gemini() const;

  payment::PostTransactionUphold* post_transaction_uphold() const;

 private:
  std::unique_ptr<payment::PostOrder> post_order_;
  std::unique_ptr<payment::PostCredentials> post_credentials_;
  std::unique_ptr<payment::GetCredentials> get_credentials_;
  std::unique_ptr<payment::PostVotes> post_votes_;
  std::unique_ptr<payment::PostTransactionGemini> post_transaction_gemini_;
  std::unique_ptr<payment::PostTransactionUphold> post_transaction_uphold_;
};

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_SERVER_H_
