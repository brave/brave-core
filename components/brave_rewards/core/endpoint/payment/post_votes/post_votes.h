/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_VOTES_POST_VOTES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_VOTES_POST_VOTES_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/credentials/credentials_redeem.h"

// POST /v1/votes
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
//   "vote": "base64_string"
// }
//
// Success code:
// HTTP_CREATED (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoint {
namespace payment {

using PostVotesCallback = base::OnceCallback<void(const mojom::Result result)>;

class PostVotes {
 public:
  explicit PostVotes(RewardsEngine& engine);
  ~PostVotes();

  void Request(const credential::CredentialsRedeem& redeem,
               PostVotesCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const credential::CredentialsRedeem& redeem);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PostVotesCallback callback, mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngine> engine_;
};

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_VOTES_POST_VOTES_H_
