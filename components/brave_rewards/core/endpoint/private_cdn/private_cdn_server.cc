/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/private_cdn/private_cdn_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace ledger {
namespace endpoint {

PrivateCDNServer::PrivateCDNServer(LedgerImpl& ledger)
    : get_publisher_(ledger) {}

PrivateCDNServer::~PrivateCDNServer() = default;

}  // namespace endpoint
}  // namespace ledger
