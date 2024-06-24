/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"

#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::endpoint {

UpholdServer::UpholdServer(RewardsEngine& engine)
    : get_capabilities_(engine),
      get_cards_(engine),
      get_card_(engine),
      get_me_(engine),
      post_cards_(engine),
      patch_card_(engine) {}

UpholdServer::~UpholdServer() = default;

}  // namespace brave_rewards::internal::endpoint
