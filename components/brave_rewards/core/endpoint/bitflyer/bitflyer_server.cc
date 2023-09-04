/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/bitflyer/bitflyer_server.h"

#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace endpoint {

BitflyerServer::BitflyerServer(RewardsEngineImpl& engine)
    : get_balance_(engine), post_oauth_(engine) {}

BitflyerServer::~BitflyerServer() = default;

}  // namespace endpoint
}  // namespace brave_rewards::internal
