/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/api/api_server.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace endpoint {

APIServer::APIServer(LedgerImpl* ledger):
    get_parameters_(std::make_unique<api::GetParameters>(ledger)) {
}

APIServer::~APIServer() = default;

api::GetParameters* APIServer::get_parameters() const {
  return get_parameters_.get();
}

}  // namespace endpoint
}  // namespace ledger
