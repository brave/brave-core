/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/payment_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace ledger {
namespace endpoint {

PaymentServer::PaymentServer(LedgerImpl& ledger)
    : post_order_(ledger),
      post_credentials_(ledger),
      get_credentials_(ledger),
      post_votes_(ledger),
      post_transaction_gemini_(ledger),
      post_transaction_uphold_(ledger) {}

PaymentServer::~PaymentServer() = default;

}  // namespace endpoint
}  // namespace ledger
