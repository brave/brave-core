/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_H_

#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal::endpoints {

// GET /v1/parameters
//
// Request body: -
//
// Response body:
// {
//   "autocontribute": {
//     "choices": [1, 2, 3, 5, 7, 10, 20],
//     "defaultChoice": 1
//   },
//   "batRate": 0.301298,
//   "custodianRegions": {
//     "bitflyer": {
//       "allow": ["JP"],
//       "block": []
//     },
//     "gemini": {
//       "allow": ["AU", "AT", "BE", "CA", "CO", "GB", "US"],
//       "block": []
//     },
//     "uphold": {
//       "allow": ["AU", "AT", "BE", "CO", "DK", "FI", "GB", "US"],
//       "block": []
//     }
//   },
//   "payoutStatus": {
//     "bitflyer": "complete",
//     "gemini": "complete",
//     "unverified": "complete",
//     "uphold": "complete"
//   },
//   "tips": {
//     "defaultMonthlyChoices": [1.25, 5, 10.5],
//     "defaultTipChoices": [1.25, 5, 10.5]
//   },
//   "vbatDeadline": "2022-12-24T15:04:45.352584Z",
//   "vbatExpired": false,
//   "tosVersion": 1
// }
class GetParameters : public RewardsEngineHelper {
 public:
  explicit GetParameters(RewardsEngineImpl& engine);
  ~GetParameters() override;

  enum class Error { kFailedToParseBody, kUnexpectedStatusCode };

  using Result = base::expected<mojom::RewardsParametersPtr, Error>;
  using RequestCallback = base::OnceCallback<void(Result)>;

  virtual void Request(RequestCallback callback);

  static std::optional<base::flat_map<std::string, mojom::RegionsPtr>>
  ValueToWalletProviderRegions(const base::Value& value);

 private:
  mojom::UrlRequestPtr CreateRequest();
  Result MapResponse(const mojom::UrlResponse& response);
  void OnResponse(RequestCallback callback, mojom::UrlResponsePtr response);

  base::WeakPtrFactory<GetParameters> weak_factory_{this};
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_H_
