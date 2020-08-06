/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/endpoint/api/api_server.h"

namespace ledger {
namespace endpoint {

APIServer::APIServer(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger),
    get_parameters_(new api::GetParameters(ledger)) {
  DCHECK(ledger_);
}

APIServer::~APIServer() = default;

api::GetParameters* APIServer::get_parameters() const {
  return get_parameters_.get();
}

}  // namespace endpoint
}  // namespace ledger
