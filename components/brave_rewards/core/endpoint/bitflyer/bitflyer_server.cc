/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/bitflyer/bitflyer_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::core {
namespace endpoint {

BitflyerServer::BitflyerServer(LedgerImpl* ledger)
    : get_balance_(std::make_unique<bitflyer::GetBalance>(ledger)),
      post_oauth_(std::make_unique<bitflyer::PostOauth>(ledger)) {}

BitflyerServer::~BitflyerServer() = default;

bitflyer::GetBalance* BitflyerServer::get_balance() const {
  return get_balance_.get();
}

bitflyer::PostOauth* BitflyerServer::post_oauth() const {
  return post_oauth_.get();
}

}  // namespace endpoint
}  // namespace brave_rewards::core
