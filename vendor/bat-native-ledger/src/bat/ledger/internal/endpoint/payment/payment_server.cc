/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/payment_server.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace endpoint {

PaymentServer::PaymentServer(LedgerImpl* ledger)
    : post_order_(std::make_unique<payment::PostOrder>(ledger)),
      post_credentials_(std::make_unique<payment::PostCredentials>(ledger)),
      get_credentials_(std::make_unique<payment::GetCredentials>(ledger)),
      post_votes_(std::make_unique<payment::PostVotes>(ledger)),
      post_transaction_gemini_(
          std::make_unique<payment::PostTransactionGemini>(ledger)),
      post_transaction_uphold_(
          std::make_unique<payment::PostTransactionUphold>(ledger)) {}

PaymentServer::~PaymentServer() = default;

payment::PostOrder* PaymentServer::post_order() const {
  return post_order_.get();
}

payment::PostCredentials* PaymentServer::post_credentials() const {
  return post_credentials_.get();
}

payment::GetCredentials* PaymentServer::get_credentials() const {
  return get_credentials_.get();
}

payment::PostVotes* PaymentServer::post_votes() const {
  return post_votes_.get();
}

payment::PostTransactionGemini* PaymentServer::post_transaction_gemini() const {
  return post_transaction_gemini_.get();
}

payment::PostTransactionUphold* PaymentServer::post_transaction_uphold() const {
  return post_transaction_uphold_.get();
}

}  // namespace endpoint
}  // namespace ledger
