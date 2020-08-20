/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/payment_server.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace endpoint {

PaymentServer::PaymentServer(bat_ledger::LedgerImpl* ledger):
    post_order_(new payment::PostOrder(ledger)),
    post_credentials_(new payment::PostCredentials(ledger)),
    get_credentials_(new payment::GetCredentials(ledger)),
    post_votes_(new payment::PostVotes(ledger)),
    post_transaction_uphold_(new payment::PostTransactionUphold(ledger)),
    post_transaction_anon_(new payment::PostTransactionAnon(ledger)) {
}

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

payment::PostTransactionUphold* PaymentServer::post_transaction_uphold() const {
  return post_transaction_uphold_.get();
}

payment::PostTransactionAnon* PaymentServer::post_transaction_anon() const {
  return post_transaction_anon_.get();
}

}  // namespace endpoint
}  // namespace ledger
