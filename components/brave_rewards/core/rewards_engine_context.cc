/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_context.h"

namespace brave_rewards::internal {

RewardsEngineContext::RewardsEngineContext(RewardsEngineImpl& engine_impl)
    : engine_impl_(engine_impl) {
  AddHelpers();
}

RewardsEngineContext::~RewardsEngineContext() {
  // Remove helpers in the reverse order in which they were added.
  for (auto iter = helper_keys_.rbegin(); iter != helper_keys_.rend(); ++iter) {
    RemoveUserData(*iter);
  }
}

void RewardsEngineContext::AddHelpers() {
  // Add all helpers here. They will be destroyed in the reverse creation order.
  //   AddHelper<HelperClass>();
}

}  // namespace brave_rewards::internal
