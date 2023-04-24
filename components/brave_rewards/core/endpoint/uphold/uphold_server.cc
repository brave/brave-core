/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace ledger {
namespace endpoint {

UpholdServer::UpholdServer(LedgerImpl& ledger)
    : get_capabilities_(ledger),
      get_cards_(ledger),
      get_card_(ledger),
      get_me_(ledger),
      post_cards_(ledger),
      patch_card_(ledger) {}

UpholdServer::~UpholdServer() = default;

}  // namespace endpoint
}  // namespace ledger
