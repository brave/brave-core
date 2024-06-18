/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/payment_server.h"

#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::endpoint {

PaymentServer::PaymentServer(RewardsEngine& engine)
    : post_order_(engine),
      post_credentials_(engine),
      get_credentials_(engine),
      post_votes_(engine),
      post_transaction_gemini_(engine),
      post_transaction_uphold_(engine) {}

PaymentServer::~PaymentServer() = default;

}  // namespace brave_rewards::internal::endpoint
