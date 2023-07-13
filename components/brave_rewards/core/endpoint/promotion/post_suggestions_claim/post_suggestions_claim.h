/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_SUGGESTIONS_CLAIM_POST_SUGGESTIONS_CLAIM_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_SUGGESTIONS_CLAIM_POST_SUGGESTIONS_CLAIM_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/credentials/credentials_redeem.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

// POST /v1/suggestions/claim
//
// Request body:
// {
//   "credentials": [
//     {
//       "t": "",
//       "publicKey": "",
//       "signature": ""
//     }
//   ],
//   "paymentId": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_SERVICE_UNAVAILABLE (503)
//
// Response body:
// {Empty}

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace promotion {

class PostSuggestionsClaim {
 public:
  explicit PostSuggestionsClaim(RewardsEngineImpl& engine);
  ~PostSuggestionsClaim();

  void Request(const credential::CredentialsRedeem& redeem,
               PostSuggestionsClaimCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const credential::CredentialsRedeem& redeem);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PostSuggestionsClaimCallback callback,
                 mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_SUGGESTIONS_CLAIM_POST_SUGGESTIONS_CLAIM_H_
