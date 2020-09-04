/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/rewards/rewards_server.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace endpoint {

RewardsServer::RewardsServer(LedgerImpl* ledger):
    get_prefix_list_(std::make_unique<rewards::GetPrefixList>(ledger)) {
}

RewardsServer::~RewardsServer() = default;

rewards::GetPrefixList* RewardsServer::get_prefix_list() const {
  return get_prefix_list_.get();
}

}  // namespace endpoint
}  // namespace ledger
