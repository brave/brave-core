/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_REWARDS_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_REWARDS_SERVER_H_

#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoint {

class RewardsServer {
 public:
  explicit RewardsServer(RewardsEngine& engine);
  ~RewardsServer();

  rewards::GetPrefixList& get_prefix_list() { return get_prefix_list_; }

 private:
  rewards::GetPrefixList get_prefix_list_;
};

}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_REWARDS_SERVER_H_
