/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CLOBBERED_CLAIMS_POST_CLOBBERED_CLAIMS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CLOBBERED_CLAIMS_POST_CLOBBERED_CLAIMS_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

// POST /v1/promotions/reportclobberedclaims
//
// Request body:
// {
//   "claimIds": ["asfeq4gerg34gl3g34lg34g"]
// }
//
// Success code:
// HTTP_OK (200)
//
// Error Codes:
// HTTP_BAD_REQUEST (400)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace promotion {

using PostClobberedClaimsCallback =
    std::function<void(const mojom::Result result)>;

class PostClobberedClaims {
 public:
  explicit PostClobberedClaims(RewardsEngineImpl& engine);
  ~PostClobberedClaims();

  void Request(base::Value::List corrupted_claims,
               PostClobberedClaimsCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(base::Value::List corrupted_claims);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PostClobberedClaimsCallback callback,
                 mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CLOBBERED_CLAIMS_POST_CLOBBERED_CLAIMS_H_
