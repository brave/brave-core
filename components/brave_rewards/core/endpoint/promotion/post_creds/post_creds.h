/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CREDS_POST_CREDS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CREDS_POST_CREDS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

// POST /v1/promotions/{promotion_id}
//
// Request body:
// {
//   "paymentId": "ff50981d-47de-4210-848d-995e186901a1",
//   "blindedCreds": [
//     "wqto9FnferrKUM0lcp2B0lecMQwArvUq3hWGCYlXiQo=",
//     "ZiSXpF61aZ/tL2MxkKzI5Vnw2aLJE2ln2FMHAtKc9Co="
//   ]
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_FORBIDDEN (403)
// HTTP_CONFLICT (409)
// HTTP_GONE (410)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "claimId": "53714048-9675-419e-baa3-369d85a2facb"
// }

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace promotion {

using PostCredsCallback =
    base::OnceCallback<void(mojom::Result result, const std::string& claim_id)>;

class PostCreds {
 public:
  explicit PostCreds(RewardsEngineImpl& engine);
  ~PostCreds();

  void Request(const std::string& promotion_id,
               base::Value::List&& blinded_creds,
               PostCredsCallback callback);

 private:
  std::string GetUrl(const std::string& promotion_id);

  std::string GeneratePayload(base::Value::List&& blinded_creds);

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body, std::string* claim_id);

  void OnRequest(PostCredsCallback callback, mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CREDS_POST_CREDS_H_
