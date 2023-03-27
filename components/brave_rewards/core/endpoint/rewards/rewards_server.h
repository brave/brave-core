/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_REWARDS_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_REWARDS_SERVER_H_

#include <memory>

#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"
#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace endpoint {

class RewardsServer {
 public:
  explicit RewardsServer(LedgerImpl* ledger);
  ~RewardsServer();

  rewards::GetPrefixList* get_prefix_list() const;

 private:
  std::unique_ptr<rewards::GetPrefixList> get_prefix_list_;
};

}  // namespace endpoint
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_REWARDS_SERVER_H_
