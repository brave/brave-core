/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/api/api.h"

#include <utility>

namespace brave_rewards::internal {
namespace api {

API::API(RewardsEngineImpl& engine) : parameters_(engine) {}

API::~API() = default;

void API::Initialize() {
  parameters_.Initialize();
}

void API::FetchParameters(GetRewardsParametersCallback callback) {
  parameters_.Fetch(std::move(callback));
}

}  // namespace api
}  // namespace brave_rewards::internal
