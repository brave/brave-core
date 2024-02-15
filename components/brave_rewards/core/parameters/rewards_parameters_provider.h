/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PARAMETERS_REWARDS_PARAMETERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PARAMETERS_REWARDS_PARAMETERS_PROVIDER_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Responsible for fetching and caching the data returned from the Rewards
// parameters endpoint, which provides global server-side configuration info.
class RewardsParametersProvider : public RewardsEngineHelper {
 public:
  explicit RewardsParametersProvider(RewardsEngineImpl& engine);
  ~RewardsParametersProvider() override;

  // Begins updating the locally-cached parameters data in the background.
  void StartAutoUpdate();

  // Returns locally-cached parameters. An empty ptr will be returned if cached
  // values do not exist.
  mojom::RewardsParametersPtr LoadParameters();

  using GetParametersCallback =
      base::OnceCallback<void(mojom::RewardsParametersPtr)>;

  // Returns the current parameter values. Fetches parameters from the Rewards
  // backend if no locally-cached data exists.
  void GetParameters(GetParametersCallback callback);

  // Converts a base::Value into RewardsParameters.
  static mojom::RewardsParametersPtr ValueToParameters(
      const base::Value& value);

  void SetParametersEndpointForTesting(
      std::unique_ptr<endpoints::GetParameters> endpoint) {
    parameters_endpoint_ = std::move(endpoint);
  }

 private:
  void Fetch(GetParametersCallback callback);

  void OnEndpointResult(endpoints::GetParameters::Result result);

  void RunCallbacks(mojom::RewardsParametersPtr parameters);

  void SetRefreshTimer(base::TimeDelta delay);

  void StoreParameters(const mojom::RewardsParameters& parameters);

  base::OneShotTimer refresh_timer_;
  std::unique_ptr<endpoints::GetParameters> parameters_endpoint_ =
      std::make_unique<endpoints::GetParameters>(engine());
  std::vector<GetParametersCallback> callbacks_;
  base::WeakPtrFactory<RewardsParametersProvider> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PARAMETERS_REWARDS_PARAMETERS_PROVIDER_H_
