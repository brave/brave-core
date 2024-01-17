/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CHALLENGES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CHALLENGES_H_

#include <optional>
#include <string>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom-shared.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"

// POST /v3/wallet/challenges
//
// Request body:
// {
//   "paymentId": "<rewards-payment-id>"
// }
//
// Success code: HTTP_CREATED (201)
//
// Response body:
// {
//   "challengeId": "<challenge-id>"
// }

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoints {

class PostChallenges;

template <>
struct ResultFor<PostChallenges> {
  using Value = std::string;
  using Error = mojom::PostChallengesError;
};

class PostChallenges final : public RequestBuilder,
                             public ResponseHandler<PostChallenges> {
 public:
  static Result ProcessResponse(const mojom::UrlResponse& response);

  explicit PostChallenges(RewardsEngineImpl& engine);
  ~PostChallenges() override;

 private:
  std::optional<std::string> Url() const override;
  std::optional<std::string> Content() const override;
  std::string ContentType() const override;
  bool NeedsToBeSigned() const override;
};
}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CHALLENGES_H_
