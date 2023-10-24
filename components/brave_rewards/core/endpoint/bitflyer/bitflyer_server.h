/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_BITFLYER_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_BITFLYER_SERVER_H_

#include "brave/components/brave_rewards/core/endpoint/bitflyer/get_balance/get_balance_bitflyer.h"
#include "brave/components/brave_rewards/core/endpoint/bitflyer/post_oauth/post_oauth_bitflyer.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {

class BitflyerServer {
 public:
  explicit BitflyerServer(RewardsEngineImpl& engine);
  ~BitflyerServer();

  bitflyer::GetBalance& get_balance() { return get_balance_; }

  bitflyer::PostOauth& post_oauth() { return post_oauth_; }

 private:
  bitflyer::GetBalance get_balance_;
  bitflyer::PostOauth post_oauth_;
};

}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_BITFLYER_SERVER_H_
