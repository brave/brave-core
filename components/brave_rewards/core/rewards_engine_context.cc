/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_context.h"

namespace brave_rewards::internal {

RewardsEngineContext::RewardsEngineContext(RewardsEngineImpl& engine_impl)
    : engine_impl_(engine_impl), helpers_(*this, *this, *this) {}

RewardsEngineContext::~RewardsEngineContext() = default;

}  // namespace brave_rewards::internal
