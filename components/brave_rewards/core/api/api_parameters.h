/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_PARAMETERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_PARAMETERS_H_

#include <memory>
#include <vector>

#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/endpoints/get_parameters/get_parameters.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::api {

class APIParameters {
 public:
  APIParameters();
  ~APIParameters();

  void Initialize();

  void Fetch(GetRewardsParametersCallback callback);

 private:
  void OnFetch(endpoints::GetParameters::Result&&);

  void RunCallbacks();

  void SetRefreshTimer(base::TimeDelta delay,
                       base::TimeDelta base_delay = base::TimeDelta());

  base::OneShotTimer refresh_timer_;
  std::vector<GetRewardsParametersCallback> callbacks_;
};

}  // namespace brave_rewards::internal::api

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_PARAMETERS_H_
