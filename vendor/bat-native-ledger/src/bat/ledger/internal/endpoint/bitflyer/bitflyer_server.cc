/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_server.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace endpoint {

BitflyerServer::BitflyerServer(LedgerImpl* ledger)
    : get_balance_(std::make_unique<bitflyer::GetBalance>(ledger)),
      post_claim_(std::make_unique<bitflyer::PostClaimBitflyer>(ledger)),
      post_oauth_(std::make_unique<bitflyer::PostOauth>(ledger)),
      post_transaction_(std::make_unique<bitflyer::PostTransaction>(ledger)) {}

BitflyerServer::~BitflyerServer() = default;

bitflyer::GetBalance* BitflyerServer::get_balance() const {
  return get_balance_.get();
}

bitflyer::PostClaimBitflyer* BitflyerServer::post_claim() const {
  return post_claim_.get();
}

bitflyer::PostOauth* BitflyerServer::post_oauth() const {
  return post_oauth_.get();
}

bitflyer::PostTransaction* BitflyerServer::post_transaction() const {
  return post_transaction_.get();
}

}  // namespace endpoint
}  // namespace ledger
