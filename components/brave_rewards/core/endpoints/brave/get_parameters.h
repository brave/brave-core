/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_H_

#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"

// GET /v1/parameters
//
// Request body: -
//
// clang-format off
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
//       "allow": ["AU", "AT", "BE", "CA", "CO", "DK", "FI", "HK", "IE", "IT", "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"],  // NOLINT
//       "block": []
//     },
//     "uphold": {
//       "allow": ["AU", "AT", "BE", "CO", "DK", "FI", "HK", "IE", "IT", "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"],  // NOLINT
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
//   "vbatExpired": false
// }
// clang-format on

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoints {

class GetParameters;

template <>
struct ResultFor<GetParameters> {
  using Value = mojom::RewardsParametersPtr;
  using Error = mojom::GetParametersError;
};

class GetParameters final : public RequestBuilder,
                            public ResponseHandler<GetParameters> {
 public:
  static Result ProcessResponse(RewardsEngine& engine,
                                const mojom::UrlResponse&);

  explicit GetParameters(RewardsEngine& engine);
  ~GetParameters() override;

  using ProviderRegionsMap = base::flat_map<std::string, mojom::RegionsPtr>;

  // Converts the specified value to a map of wallet provider type to supported
  // region data. Returns `std::nullopt` if the value is not in the correct
  // format and cannot be converted.
  static std::optional<ProviderRegionsMap> ValueToWalletProviderRegions(
      const base::Value& value);

 private:
  std::optional<std::string> Url() const override;
  mojom::UrlMethod Method() const override;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_H_
