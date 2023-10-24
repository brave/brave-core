/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_H_

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/api/api_parameters.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace api {

class API {
 public:
  explicit API(RewardsEngineImpl& engine);
  ~API();

  void Initialize();

  void FetchParameters(GetRewardsParametersCallback callback);

 private:
  APIParameters parameters_;
};

}  // namespace api
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_H_
