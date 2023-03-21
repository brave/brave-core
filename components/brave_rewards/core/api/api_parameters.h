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

namespace ledger {
class LedgerImpl;

namespace api {

class APIParameters {
 public:
  explicit APIParameters(LedgerImpl* ledger);
  ~APIParameters();

  void Initialize();

  void Fetch(ledger::GetRewardsParametersCallback callback);

 private:
  void OnFetch(endpoints::GetParameters::Result&&);

  void RunCallbacks();

  void SetRefreshTimer(base::TimeDelta delay,
                       base::TimeDelta base_delay = base::TimeDelta());

  LedgerImpl* ledger_;  // NOT OWNED
  base::OneShotTimer refresh_timer_;
  std::vector<ledger::GetRewardsParametersCallback> callbacks_;
};

}  // namespace api
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_PARAMETERS_H_
