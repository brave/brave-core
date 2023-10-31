/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_SERVER_H_

#include "brave/components/brave_rewards/core/endpoint/payment/get_credentials/get_credentials.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_credentials/post_credentials.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_order/post_order.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_uphold/post_transaction_uphold.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_votes/post_votes.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {

class PaymentServer {
 public:
  explicit PaymentServer(RewardsEngineImpl& engine);
  ~PaymentServer();

  payment::PostOrder& post_order() { return post_order_; }

  payment::PostCredentials& post_credentials() { return post_credentials_; }

  payment::GetCredentials& get_credentials() { return get_credentials_; }

  payment::PostVotes& post_votes() { return post_votes_; }

  payment::PostTransactionGemini& post_transaction_gemini() {
    return post_transaction_gemini_;
  }

  payment::PostTransactionUphold& post_transaction_uphold() {
    return post_transaction_uphold_;
  }

 private:
  payment::PostOrder post_order_;
  payment::PostCredentials post_credentials_;
  payment::GetCredentials get_credentials_;
  payment::PostVotes post_votes_;
  payment::PostTransactionGemini post_transaction_gemini_;
  payment::PostTransactionUphold post_transaction_uphold_;
};

}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_SERVER_H_
