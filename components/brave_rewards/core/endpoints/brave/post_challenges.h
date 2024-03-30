/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CHALLENGES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CHALLENGES_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal::endpoints {

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
class PostChallenges : public RewardsEngineHelper,
                       public WithHelperKey<PostChallenges> {
 public:
  explicit PostChallenges(RewardsEngine& engine);
  ~PostChallenges() override;

  enum class Error {
    kFailedToCreateRequest,
    kUnexpectedStatusCode,
    kFailedToParseBody
  };

  using Result = base::expected<std::string, Error>;
  using RequestCallback = base::OnceCallback<void(Result result)>;

  virtual void Request(RequestCallback callback);

 private:
  mojom::UrlRequestPtr CreateRequest();
  Result MapResponse(const mojom::UrlResponse& response);
  void OnResponse(RequestCallback callback, mojom::UrlResponsePtr response);

  base::WeakPtrFactory<PostChallenges> weak_factory_{this};
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CHALLENGES_H_
